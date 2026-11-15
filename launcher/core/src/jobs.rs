//! Running actions. A job is a child process whose output lines go to a
//! sink as [`Event`]s (the app forwards them to the UI as `job-output` and
//! `job-exit`); a detached launch (the game, Godot) is started and left
//! alone.

use std::collections::{HashMap, HashSet, VecDeque};
use std::io::{BufRead, BufReader, Read};
use std::process::{Child, Command, Stdio};
use std::sync::atomic::{AtomicU32, Ordering};
use std::sync::{Arc, Condvar, Mutex};
use std::time::{Duration, Instant};

use serde::Serialize;

use crate::actions::Plan;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, ts_rs::TS)]
#[ts(rename = "JobEvent")]
#[serde(rename_all = "camelCase", tag = "type")]
pub enum Event {
    Output {
        id: u32,
        stream: Stream,
        line: String,
    },
    /// `outputs`: the folders the job wrote, as the tool named them on its `openrac-output:` lines,
    /// else the action's `output`; empty when it names none.
    Exit {
        id: u32,
        code: Option<i32>,
        cancelled: bool,
        outputs: Vec<String>,
    },
}

/// A line saying which signal ended a program, or None when it exited by itself. It is the
/// only trace of a program the system killed (Android ends one out of memory with SIGKILL).
fn stopped_by_signal(status: &std::process::ExitStatus) -> Option<String> {
    #[cfg(unix)]
    {
        use std::os::unix::process::ExitStatusExt;
        let sig = status.signal()?;
        // SIGBUS is 7 on Linux and Android, 10 on macOS; the others match.
        const SIGBUS: i32 = if cfg!(target_vendor = "apple") { 10 } else { 7 };
        let why = match sig {
            9 => "SIGKILL: the system stopped it, most often because memory ran out",
            6 => "SIGABRT: it aborted",
            11 => "SIGSEGV: it crashed",
            SIGBUS => "SIGBUS: it crashed",
            4 => "SIGILL: it crashed",
            8 => "SIGFPE: it crashed",
            15 => "SIGTERM: something asked it to stop",
            _ => "",
        };
        Some(if why.is_empty() {
            format!("openrac-launcher: the program was stopped by signal {sig}")
        } else {
            format!("openrac-launcher: the program was stopped by signal {sig} ({why})")
        })
    }
    #[cfg(not(unix))]
    {
        let _ = status;
        None
    }
}

/// The start of a line on which a tool names a folder it wrote (editor/extract.py, assets.py).
pub const OUTPUT_LINE: &str = "openrac-output: ";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, ts_rs::TS)]
#[ts(rename = "JobStream")]
#[serde(rename_all = "camelCase")]
pub enum Stream {
    Stdout,
    Stderr,
}

/// How long the end of a job waits for its output to be read, after the program itself has
/// ended: long enough for the last lines, short enough that a program that left something
/// behind (a daemon, a watcher) does not hold the queue up for ever.
const OUTPUT_GRACE: Duration = Duration::from_secs(3);

#[derive(Debug, Clone, Serialize, ts_rs::TS)]
#[serde(rename_all = "camelCase")]
pub struct Started {
    /// 0 for a detached launch, which has no job.
    pub id: u32,
    pub title: String,
    /// The command line, for the Tasks page and bug reports.
    pub command: String,
    pub detached: bool,
    /// The folder the action writes in (its `output`), when it names one.
    pub output: Option<String>,
}

/// The running jobs. Cheap to clone; clones share the same jobs.
#[derive(Clone, Default)]
pub struct Jobs {
    inner: Arc<Inner>,
}

#[derive(Default)]
struct Inner {
    next: AtomicU32,
    /// The running jobs and the ones the player cancelled, under one lock: a cancel and
    /// the end of a job are decided together, so that one cannot come apart from the other.
    jobs: Mutex<State>,
}

#[derive(Default)]
struct State {
    running: HashMap<u32, Arc<Mutex<Child>>>,
    cancelled: HashSet<u32>,
}

/// What every job gets in its environment: unbuffered, UTF-8 Python output, no `__pycache__`
/// (an installed launcher's copy is inside its own bundle: the macOS app in /Applications, the
/// AppImage, /usr/lib for a package), and the checkout, for tools that want it.
pub(crate) fn environment(cmd: &mut Command, root: Option<&std::path::Path>) {
    // An AppImage's own environment (the AppRun's PYTHONHOME, its paths, its
    // libraries) would follow the job: a Python told that its home is the
    // AppImage's cannot start, and the game would find the AppImage's
    // libraries instead of the system's. The job gets the system's.
    environment_with(cmd, root, crate::appimage::appdir().as_deref());
}

/// [`environment`] for an AppImage mounted at `appdir` (`None`: not from one).
fn environment_with(cmd: &mut Command, root: Option<&std::path::Path>, appdir: Option<&std::path::Path>) {
    cmd.env("PYTHONUNBUFFERED", "1").env("PYTHONIOENCODING", "utf-8").env("PYTHONDONTWRITEBYTECODE", "1");
    if let Some(root) = root {
        cmd.env("OPENRAC_ROOT", root);
    }
    if let Some(appdir) = appdir {
        crate::appimage::sanitize_with(cmd, appdir);
    }
}

impl Jobs {
    /// Starts `plan` (a detached one through [`launch`]). `sink` gets every
    /// output line and, last, the exit; it is called from other threads.
    pub fn start(
        &self,
        plan: &Plan,
        root: Option<&std::path::Path>,
        sink: impl Fn(Event) + Send + Sync + 'static,
    ) -> Result<Started, String> {
        if plan.detached {
            return launch(plan, root);
        }
        let mut cmd = crate::detect::quiet(&plan.program);
        cmd.args(&plan.args).current_dir(&plan.cwd).stdin(Stdio::null()).stdout(Stdio::piped()).stderr(Stdio::piped());
        environment(&mut cmd, root);
        cmd.envs(plan.env.iter().map(|(k, v)| (k, v)));
        own_group(&mut cmd);
        let mut child = cmd.spawn().map_err(|e| format!("cannot start {}: {e}", plan.program.display()))?;

        let id = self.inner.next.fetch_add(1, Ordering::Relaxed) + 1;
        let named: Arc<Mutex<Vec<String>>> = Arc::default();
        let sink = {
            let named = named.clone();
            Arc::new(move |event: Event| {
                if let Event::Output { line, .. } = &event {
                    if let Some(path) = line.strip_prefix(OUTPUT_LINE) {
                        let mut named = named.lock().unwrap();
                        if !named.iter().any(|p| p == path.trim()) {
                            named.push(path.trim().to_string());
                        }
                    }
                }
                sink(event)
            })
        };
        let mut pumps = Vec::new();
        if let Some(out) = child.stdout.take() {
            pumps.push(pump(id, Stream::Stdout, out, sink.clone()));
        }
        if let Some(err) = child.stderr.take() {
            pumps.push(pump(id, Stream::Stderr, err, sink.clone()));
        }
        let declared = plan.output.as_ref().map(|p| p.display().to_string());
        let child = Arc::new(Mutex::new(child));
        self.inner.jobs.lock().unwrap().running.insert(id, child.clone());

        let inner = self.inner.clone();
        std::thread::spawn(move || {
            // Poll rather than wait(), so cancel() can take the lock and kill.
            let status = loop {
                match child.lock().unwrap().try_wait() {
                    Ok(Some(status)) => break Some(status),
                    Ok(None) => {}
                    Err(_) => break None,
                }
                std::thread::sleep(Duration::from_millis(100));
            };
            // The output is read by two threads, which end when the pipe is closed. A
            // program that leaves something behind (a daemon, a watcher) keeps the pipe
            // open after it has ended, so the end of the job is reported after a moment:
            // the tail of the output may be lost, but a process that is not the job's
            // never holds the queue up.
            let deadline = Instant::now() + OUTPUT_GRACE;
            for p in pumps {
                while !p.is_finished() && Instant::now() < deadline {
                    std::thread::sleep(Duration::from_millis(20));
                }
                if p.is_finished() {
                    let _ = p.join();
                }
                // Not finished: the thread ends when the pipe is closed, at the latest when
                // the program holding it ends. It is left to end on its own.
            }
            let cancelled = {
                let mut jobs = inner.jobs.lock().unwrap();
                jobs.running.remove(&id);
                jobs.cancelled.remove(&id)
            };
            let mut outputs = named.lock().unwrap().clone();
            if outputs.is_empty() {
                outputs.extend(declared);
            }
            if let Some(line) = status.as_ref().filter(|_| !cancelled).and_then(stopped_by_signal) {
                sink(Event::Output { id, stream: Stream::Stderr, line });
            }
            sink(Event::Exit { id, code: status.and_then(|s| s.code()), cancelled, outputs });
        });
        Ok(Started {
            id,
            title: plan.title.clone(),
            command: command_line(plan),
            detached: false,
            output: plan.output.as_ref().map(|p| p.display().to_string()),
        })
    }

    /// Stops job `id` and everything it started (a build runs compilers in
    /// containers). A job that has already ended is not an error.
    pub fn cancel(&self, id: u32) -> Result<(), String> {
        // The end of a job and the cancel of it are decided together: a cancel that
        // arrives as the job ends is either the one that stopped it (the exit says so) or
        // nothing at all (the job had already ended). They cannot come apart, and one
        // cannot leave the other behind.
        let child = {
            let mut jobs = self.inner.jobs.lock().unwrap();
            match jobs.running.get(&id).cloned() {
                Some(child) => {
                    jobs.cancelled.insert(id);
                    Some(child)
                }
                None => None,
            }
        };
        let Some(child) = child else { return Ok(()) };
        let mut child = child.lock().unwrap();
        kill_tree(&mut child)
    }

    pub fn running(&self) -> Vec<u32> {
        let mut ids: Vec<u32> = self.inner.jobs.lock().unwrap().running.keys().copied().collect();
        ids.sort_unstable();
        ids
    }
}

/// How often a job's output goes to the page. One event per line floods the window: each event
/// is a call into the web view on the launcher's main thread, so a chatty program (the game, the
/// baker) froze the window and its lines kept coming long after the program had ended.
const BATCH_EVERY: Duration = Duration::from_millis(100);

/// The most lines one batch carries. A program that writes more in [`BATCH_EVERY`] loses its
/// older lines (a note says how many); the page keeps only its last lines anyway.
const BATCH_MOST: usize = 1000;

/// A sink for [`Jobs::start`] that hands `deliver` the job's events in batches, a few times a
/// second: the output lines so far, and the exit last, after the lines before it. Lines that come
/// after the exit (from a process the job left behind) are dropped, as the job has ended.
pub fn batched(deliver: impl Fn(Vec<Event>) + Send + Sync + 'static) -> impl Fn(Event) + Send + Sync + 'static {
    batched_every(BATCH_EVERY, deliver)
}

/// [`batched`] with its batch time, so the tests can choose one.
fn batched_every(
    every: Duration,
    deliver: impl Fn(Vec<Event>) + Send + Sync + 'static,
) -> impl Fn(Event) + Send + Sync + 'static {
    let shared = Arc::new(Batch { pending: Mutex::default(), wake: Condvar::new(), deliver: Box::new(deliver) });
    let flusher = shared.clone();
    std::thread::spawn(move || loop {
        let ended = {
            // Asleep while the job writes nothing (a game that logs little), then a batch's time
            // to gather more lines once one came.
            let pending = flusher.pending.lock().unwrap();
            let pending = flusher.wake.wait_while(pending, |p| p.lines.is_empty() && !p.ended).unwrap();
            let (pending, _) = flusher.wake.wait_timeout_while(pending, every, |p| !p.ended).unwrap();
            pending.ended
        };
        flusher.flush(None);
        if ended {
            break;
        }
    });
    let sink = BatchSink(shared);
    move |event| sink.push(event)
}

struct Batch {
    pending: Mutex<Pending>,
    /// Woken by a job's first line after a batch, by its exit and by the sink's end.
    wake: Condvar,
    deliver: Box<dyn Fn(Vec<Event>) + Send + Sync>,
}

#[derive(Default)]
struct Pending {
    lines: VecDeque<Event>,
    /// Lines dropped since the last batch, as more came than one batch carries.
    dropped: usize,
    /// The exit was delivered (or the sink is gone): nothing more is.
    ended: bool,
}

impl Batch {
    /// Delivers the lines so far, then `exit`. One delivery at a time, so the order holds.
    fn flush(&self, exit: Option<Event>) {
        let mut pending = self.pending.lock().unwrap();
        // A job ends once: an exit after the end (or after the sink is gone) is not delivered.
        if pending.ended && exit.is_some() {
            return;
        }
        let mut events: Vec<Event> = pending.lines.drain(..).collect();
        if pending.dropped > 0 {
            if let Some(Event::Output { id, .. }) = events.first() {
                let line = format!("openrac-launcher: {} lines left out (written faster than shown)", pending.dropped);
                events.insert(0, Event::Output { id: *id, stream: Stream::Stderr, line });
            }
            pending.dropped = 0;
        }
        if let Some(exit) = exit {
            pending.ended = true;
            events.push(exit);
            self.wake.notify_all();
        }
        // Delivered under the lock: the flusher and the exit cannot overtake each other.
        if !events.is_empty() {
            (self.deliver)(events);
        }
    }
}

/// The sink's end of a [`batched`] sink: when it is dropped (the job ended, or never started) the
/// flusher thread ends too.
struct BatchSink(Arc<Batch>);

impl BatchSink {
    fn push(&self, event: Event) {
        match event {
            Event::Output { .. } => {
                let mut pending = self.0.pending.lock().unwrap();
                if pending.ended {
                    return;
                }
                if pending.lines.len() == BATCH_MOST {
                    pending.lines.pop_front();
                    pending.dropped += 1;
                }
                pending.lines.push_back(event);
                if pending.lines.len() == 1 {
                    self.0.wake.notify_all();
                }
            }
            exit @ Event::Exit { .. } => self.0.flush(Some(exit)),
        }
    }
}

impl Drop for BatchSink {
    fn drop(&mut self) {
        self.0.pending.lock().unwrap().ended = true;
        self.0.wake.notify_all();
    }
}

/// Starts `plan` on its own: its output is not read and the launcher does
/// not wait for it.
pub fn launch(plan: &Plan, root: Option<&std::path::Path>) -> Result<Started, String> {
    let mut child = spawn_detached(plan, root)?;
    // Reap it when it ends, so it never lingers as a zombie on Unix.
    std::thread::spawn(move || child.wait());
    Ok(Started {
        id: 0,
        title: plan.title.clone(),
        command: command_line(plan),
        detached: true,
        output: plan.output.as_ref().map(|p| p.display().to_string()),
    })
}

/// Starts a detached plan and hands back the child, for a caller that waits for it (a game started
/// from a shortcut, which ends the launcher with it).
pub fn spawn_detached(plan: &Plan, root: Option<&std::path::Path>) -> Result<std::process::Child, String> {
    // Quiet, so a console program (Godot's `_console.exe`, a Scoop shim) opens no console
    // window on Windows; a window program opens its window as ever.
    let mut cmd = crate::detect::quiet(&plan.program);
    cmd.args(&plan.args).current_dir(&plan.cwd).stdin(Stdio::null()).stdout(Stdio::null()).stderr(Stdio::null());
    environment(&mut cmd, root);
    cmd.envs(plan.env.iter().map(|(k, v)| (k, v)));
    cmd.spawn().map_err(|e| format!("cannot start {}: {e}", plan.program.display()))
}

/// The command line as a person would type it, for display only.
pub fn command_line(plan: &Plan) -> String {
    let quote = |s: &str| {
        if s.is_empty() || s.contains([' ', '"', '\'']) {
            format!("\"{}\"", s.replace('"', "\\\""))
        } else {
            s.to_string()
        }
    };
    std::iter::once(plan.program.display().to_string())
        .chain(plan.args.iter().cloned())
        .map(|s| quote(&s))
        .collect::<Vec<_>>()
        .join(" ")
}

#[cfg(unix)]
fn own_group(cmd: &mut Command) {
    use std::os::unix::process::CommandExt;
    cmd.process_group(0);
}

#[cfg(not(unix))]
fn own_group(_: &mut Command) {}

#[cfg(unix)]
fn kill_tree(child: &mut Child) -> Result<(), String> {
    // The job leads its own process group (own_group): signal all of it.
    let group = format!("-{}", child.id());
    let killed = Command::new("kill").args(["-TERM", "--", &group]).status().is_ok_and(|s| s.success());
    if killed {
        return Ok(());
    }
    child.kill().map_err(|e| e.to_string())
}

#[cfg(windows)]
fn kill_tree(child: &mut Child) -> Result<(), String> {
    let mut kill = crate::detect::quiet(std::path::Path::new("taskkill"));
    kill.args(["/T", "/F", "/PID", &child.id().to_string()]);
    if crate::detect::run(kill, Duration::from_secs(10)).is_ok() {
        return Ok(());
    }
    child.kill().map_err(|e| e.to_string())
}

/// Sends each line of `stream` to `sink`. Tools redraw progress with `\r`,
/// so that splits lines too; terminal colours are removed.
fn pump(
    id: u32,
    stream: Stream,
    reader: impl Read + Send + 'static,
    sink: Arc<impl Fn(Event) + Send + Sync + 'static>,
) -> std::thread::JoinHandle<()> {
    std::thread::spawn(move || {
        let mut reader = BufReader::new(reader);
        let mut buf = Vec::new();
        loop {
            buf.clear();
            match reader.read_until(b'\n', &mut buf) {
                Ok(0) | Err(_) => break,
                Ok(_) => {
                    let text = String::from_utf8_lossy(&buf);
                    for line in text.trim_end_matches(['\r', '\n']).split('\r') {
                        sink(Event::Output { id, stream, line: plain(line) });
                    }
                }
            }
        }
    })
}

/// `line` without ANSI escape sequences (colours, cursor moves, titles).
pub fn plain(line: &str) -> String {
    let mut out = String::with_capacity(line.len());
    let mut chars = line.chars().peekable();
    while let Some(c) = chars.next() {
        if c != '\x1b' {
            out.push(c);
            continue;
        }
        match chars.next() {
            // CSI: parameters and intermediates, ended by a byte from @ to ~.
            Some('[') => {
                for c in chars.by_ref() {
                    if ('@'..='~').contains(&c) {
                        break;
                    }
                }
            }
            // OSC: ended by BEL or by ESC \.
            Some(']') => {
                while let Some(c) = chars.next() {
                    if c == '\x07' {
                        break;
                    }
                    if c == '\x1b' {
                        chars.next();
                        break;
                    }
                }
            }
            // Two-byte and charset escapes (ESC ( B): skip intermediates and the final byte.
            Some(c) if (' '..='/').contains(&c) => {
                while chars.next_if(|c| (' '..='/').contains(c)).is_some() {}
                chars.next();
            }
            _ => {}
        }
    }
    out
}

#[cfg(test)]
mod tests {
    use super::*;
    // For the tests that run a shell, which are Unix only.
    #[cfg(unix)]
    use std::path::PathBuf;
    #[cfg(unix)]
    use std::sync::mpsc;

    #[test]
    fn plain_removes_escapes() {
        assert_eq!(plain("\x1b[1;32mok\x1b[0m done"), "ok done");
        assert_eq!(plain("\x1b]0;title\x07after \x1b(Bé"), "after é");
        assert_eq!(plain("nothing to strip"), "nothing to strip");
    }

    #[test]
    fn command_lines_quote_what_needs_it() {
        let plan = Plan {
            title: "t".into(),
            program: "python3".into(),
            args: vec!["tools/openrac.py".into(), "my disc.iso".into(), "".into()],
            cwd: ".".into(),
            detached: false,
            env: Vec::new(),
            output: None,
        };
        assert_eq!(command_line(&plan), "python3 tools/openrac.py \"my disc.iso\" \"\"");
    }

    #[cfg(unix)]
    fn sh(script: &str) -> Plan {
        Plan {
            title: "sh".into(),
            program: PathBuf::from("/bin/sh"),
            args: vec!["-c".into(), script.into()],
            cwd: std::env::temp_dir(),
            detached: false,
            env: Vec::new(),
            output: None,
        }
    }

    #[cfg(unix)]
    #[test]
    fn streams_output_then_the_exit() {
        let jobs = Jobs::default();
        let (tx, rx) = mpsc::channel();
        let tx = Mutex::new(tx);
        let started = jobs
            .start(&sh("echo one; printf 'a\\rb\\n'; echo two >&2; exit 3"), None, move |e| {
                tx.lock().unwrap().send(e).unwrap();
            })
            .unwrap();
        let mut lines = Vec::new();
        let exit = loop {
            match rx.recv_timeout(Duration::from_secs(10)).unwrap() {
                Event::Output { line, stream, .. } => lines.push((stream, line)),
                exit @ Event::Exit { .. } => break exit,
            }
        };
        assert_eq!(exit, Event::Exit { id: started.id, code: Some(3), cancelled: false, outputs: vec![] });
        let stdout: Vec<_> = lines.iter().filter(|(s, _)| *s == Stream::Stdout).map(|(_, l)| l.as_str()).collect();
        assert_eq!(stdout, ["one", "a", "b"]);
        assert!(lines.contains(&(Stream::Stderr, "two".to_string())));
        assert!(jobs.running().is_empty());
    }

    #[cfg(unix)]
    #[test]
    fn a_killed_program_says_which_signal() {
        let jobs = Jobs::default();
        let (tx, rx) = mpsc::channel();
        let tx = Mutex::new(tx);
        jobs.start(&sh("kill -9 $$"), None, move |e| {
            tx.lock().unwrap().send(e).unwrap();
        })
        .unwrap();
        let mut lines = Vec::new();
        let code = loop {
            match rx.recv_timeout(Duration::from_secs(10)).unwrap() {
                Event::Output { line, .. } => lines.push(line),
                Event::Exit { code, .. } => break code,
            }
        };
        assert_eq!(code, None);
        assert!(lines.iter().any(|l| l.contains("signal 9 (SIGKILL")), "{lines:?}");
    }

    #[cfg(unix)]
    #[test]
    fn the_exit_names_the_folders_the_job_wrote() {
        let jobs = Jobs::default();
        let run = |plan: Plan| {
            let (tx, rx) = mpsc::channel();
            let tx = Mutex::new(tx);
            jobs.start(&plan, None, move |e| tx.lock().unwrap().send(e).unwrap()).unwrap();
            loop {
                if let Event::Exit { mut outputs, .. } = rx.recv_timeout(Duration::from_secs(30)).unwrap() {
                    // stdout and stderr are read by two threads: the order across them is not defined.
                    outputs.sort();
                    break outputs;
                }
            }
        };
        // The tool names them: those, once each.
        let mut plan = sh("echo 'openrac-output: /a/b'; echo 'openrac-output: /a/b'; echo 'openrac-output: /c' >&2");
        plan.output = Some(PathBuf::from("/declared"));
        assert_eq!(run(plan), ["/a/b", "/c"]);
        // It names none: the action's output.
        let mut plan = sh("echo done");
        plan.output = Some(PathBuf::from("/declared"));
        assert_eq!(run(plan), ["/declared"]);
        assert!(run(sh("true")).is_empty());
    }

    #[cfg(unix)]
    #[test]
    fn cancel_stops_the_whole_job() {
        let jobs = Jobs::default();
        let (tx, rx) = mpsc::channel();
        let tx = Mutex::new(tx);
        // The child shell starts a grandchild: both must go.
        let started = jobs
            .start(&sh("echo started; sleep 30 & wait"), None, move |e| {
                tx.lock().unwrap().send(e).unwrap();
            })
            .unwrap();
        assert!(matches!(rx.recv_timeout(Duration::from_secs(10)).unwrap(), Event::Output { .. }));
        jobs.cancel(started.id).unwrap();
        let exit = rx.recv_timeout(Duration::from_secs(10)).unwrap();
        assert!(matches!(exit, Event::Exit { cancelled: true, .. }), "{exit:?}");
        // The cancel and the end of the job are decided together: nothing is left behind.
        assert!(jobs.inner.jobs.lock().unwrap().cancelled.is_empty());
        assert!(jobs.running().is_empty());
    }

    #[cfg(unix)]
    #[test]
    fn a_job_that_leaves_a_process_behind_still_ends() {
        let jobs = Jobs::default();
        let (tx, rx) = mpsc::channel();
        let tx = Mutex::new(tx);
        // The program ends at once, but the process it left behind keeps the output pipe
        // open: the job must still report its end, or the queue waits for ever.
        let started = jobs
            .start(&sh("sleep 10 & echo started; exit 0"), None, move |e| {
                tx.lock().unwrap().send(e).unwrap();
            })
            .unwrap();
        let mut lines = Vec::new();
        let exit = loop {
            match rx.recv_timeout(Duration::from_secs(8)).unwrap() {
                Event::Output { line, .. } => lines.push(line),
                exit @ Event::Exit { .. } => break exit,
            }
        };
        assert_eq!(exit, Event::Exit { id: started.id, code: Some(0), cancelled: false, outputs: vec![] });
        assert!(lines.contains(&"started".to_string()), "{lines:?}");
    }

    #[test]
    fn batches_lines_then_the_exit() {
        let (tx, rx) = std::sync::mpsc::channel();
        let tx = Mutex::new(tx);
        // A batch time far longer than the test: the lines and the exit arrive as one batch.
        let sink = batched_every(Duration::from_secs(3600), move |batch| tx.lock().unwrap().send(batch).unwrap());
        let line = |n: usize| Event::Output { id: 1, stream: Stream::Stdout, line: n.to_string() };
        for n in 0..BATCH_MOST + 5 {
            sink(line(n));
        }
        let exit = Event::Exit { id: 1, code: Some(0), cancelled: false, outputs: vec![] };
        sink(exit.clone());
        // Lines after the end are from something the job left behind: not shown.
        sink(line(9999));
        drop(sink);
        let batches: Vec<Vec<Event>> = rx.iter().collect();
        let events: Vec<Event> = batches.iter().flatten().cloned().collect();
        assert_eq!(batches.len(), 1);
        assert_eq!(events.last(), Some(&exit));
        // The oldest lines past the batch's size are dropped, and a note says so.
        let Event::Output { line: note, .. } = &events[0] else { panic!("{:?}", events[0]) };
        assert_eq!(note, "openrac-launcher: 5 lines left out (written faster than shown)");
        assert_eq!(events[1], line(5));
        assert_eq!(events.len(), BATCH_MOST + 2);
    }

    #[test]
    fn batches_lines_while_the_job_runs() {
        let (tx, rx) = std::sync::mpsc::channel();
        let tx = Mutex::new(tx);
        let sink = batched(move |batch| tx.lock().unwrap().send(batch).unwrap());
        sink(Event::Output { id: 2, stream: Stream::Stderr, line: "working".into() });
        // No exit yet: the line still comes, within a batch's time.
        let batch = rx.recv_timeout(Duration::from_secs(5)).unwrap();
        assert_eq!(batch, [Event::Output { id: 2, stream: Stream::Stderr, line: "working".into() }]);
        drop(sink);
    }

    #[cfg(unix)]
    #[test]
    fn a_chatty_job_comes_in_few_batches() {
        let jobs = Jobs::default();
        let (tx, rx) = mpsc::channel();
        let tx = Mutex::new(tx);
        let started = jobs
            .start(
                &sh("i=0; while [ $i -lt 3000 ]; do echo $i; i=$((i+1)); done"),
                None,
                batched(move |batch| {
                    tx.lock().unwrap().send(batch).unwrap();
                }),
            )
            .unwrap();
        let mut batches = 0;
        let mut lines = Vec::new();
        let exit = 'job: loop {
            batches += 1;
            for event in rx.recv_timeout(Duration::from_secs(30)).unwrap() {
                match event {
                    Event::Output { line, .. } => lines.push(line),
                    exit @ Event::Exit { .. } => break 'job exit,
                }
            }
        };
        assert_eq!(exit, Event::Exit { id: started.id, code: Some(0), cancelled: false, outputs: vec![] });
        // Far fewer events than lines, and every line there, in order (none was dropped at this pace
        // unless a batch overflowed, which the note would say).
        assert!(batches < 3000 / 10, "{batches} batches");
        if !lines.iter().any(|l| l.contains("lines left out")) {
            assert_eq!(lines, (0..3000).map(|n| n.to_string()).collect::<Vec<_>>());
        }
    }

    /// The AppImage is given as an argument, not set in this process's environment: the tests
    /// run in parallel, and a PYTHONHOME pointing at nothing would follow every Python another
    /// test starts meanwhile.
    #[test]
    fn cleans_appimage_python_environment() {
        let mut cmd = Command::new("true");
        cmd.env("PYTHONHOME", "/tmp/.mount_test/usr/");
        cmd.env("PYTHONPATH", "/tmp/.mount_test/usr/share/pyshared/:");
        environment_with(&mut cmd, None, Some(std::path::Path::new("/tmp/.mount_test")));
        let envs: HashMap<_, _> = cmd.get_envs().collect();
        assert_eq!(envs.get(std::ffi::OsStr::new("PYTHONHOME")), Some(&None));
        assert_eq!(envs.get(std::ffi::OsStr::new("PYTHONPATH")), Some(&None));
    }
}
