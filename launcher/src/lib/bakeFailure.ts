import type { Job } from "./queue";

export interface BakeFailure {
  version: 1;
  code: string;
  title: string;
  explanation: string;
  suggestion: string;
  detail: string;
  stage?: string | null;
  level?: string | null;
  log?: string | null;
  failed_levels?: number;
}

const PREFIX = "openrac-bake-error: ";

/** Only the bake job's data records become notices. Text and paths are never executable UI. */
export function bakeFailure(job: Pick<Job, "lines" | "code">): BakeFailure {
  for (const { line } of [...job.lines].reverse()) {
    if (!line.startsWith(PREFIX)) continue;
    try {
      const data: unknown = JSON.parse(line.slice(PREFIX.length));
      if (!data || typeof data !== "object") continue;
      const record = data as Record<string, unknown>;
      if (record.version !== 1) continue;
      if (!["code", "title", "explanation", "suggestion", "detail"].every((key) => typeof record[key] === "string"))
        continue;
      return {
        version: 1,
        code: String(record.code).slice(0, 80),
        title: String(record.title).slice(0, 240),
        explanation: String(record.explanation).slice(0, 1000),
        suggestion: String(record.suggestion).slice(0, 1000),
        detail: String(record.detail).slice(-12000),
        level: typeof record.level === "string" ? record.level : null,
        stage: typeof record.stage === "string" ? record.stage : null,
        log: typeof record.log === "string" ? record.log : null,
        failed_levels: typeof record.failed_levels === "number" ? record.failed_levels : undefined,
      };
    } catch {
      // A truncated or older tool's record must not prevent the failure notice itself.
    }
  }
  return {
    version: 1,
    code: "unreported",
    title: "Lighting preparation could not finish",
    explanation: "The lighting tool stopped without a detailed error report.",
    suggestion: "Open Activity for its output. Check you have the latest launcher, then retry or send us the output.",
    detail: `Exit status: ${job.code ?? "unavailable"}\n${job.lines
      .slice(-20)
      .map(({ line }) => line)
      .join("\n")}`,
  };
}
