# Antivirus false positives

OpenRAC's Windows files are not code signed. Some antivirus programs flag new, unsigned files
by guesswork (heuristics), not because they found real malware. A name like
`Gen:Variant.Application...` or "potentially unwanted application" (PUA) is that kind of guess.

`openrac-shaders.dll` is easy to misjudge: it is about 100 MB of compressed shader data with one
small function. Packed malware looks similar from the outside. The port's Windows programs and
libraries carry a version resource (name, version, licence) to look less odd, but only the
antivirus vendor can clear a detection.

## If your antivirus removes a file

1. Restore the file from quarantine and add an exclusion for the OpenRAC folder.
2. Report the false positive to the vendor (below). This helps every user.

## Report it to Bitdefender

1. Get the file's SHA-256. In PowerShell:

   ```powershell
   Get-FileHash "C:\path\to\openrac-shaders.dll" -Algorithm SHA256
   ```

2. Put the file in a `.zip`. The form takes files up to 25 MB, so a bigger file may need a
   download link instead (a GitHub release asset works).
3. Open <https://www.bitdefender.com/submit/> and pick **False positive**.
4. Fill in:
   - the detection name exactly as Bitdefender shows it, for example
     `Gen:Variant.Application.FCA.4519`;
   - the file name, the SHA-256 and the OpenRAC version (the file's
     Properties > Details tab);
   - a short note, for example: "Open-source game port, GPL-3.0. This DLL holds compiled Vulkan
     shaders. Source: https://github.com/OpenRAC/OpenRAC";
   - a screenshot of the detection, if you have one.
5. Send it. Bitdefender usually answers by email. Once they agree, the fix reaches users with
   the next signature update.

Each new build has a new SHA-256, so a new build can be flagged again. Report it again with the
new hash if that happens.

Other vendors have similar forms. Microsoft Defender:
<https://www.microsoft.com/en-us/wdsi/filesubmission>.
