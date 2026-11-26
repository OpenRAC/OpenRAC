# Antivirus false positives

OpenRAC's Windows files are not code signed. Some antivirus programs flag new, unsigned files
by guesswork, not because they found malware. Names like `Gen:Variant...`, "PUA" or
"potentially unwanted application" usually mean a guess.

## What to do

1. Restore the file from quarantine and add an exclusion for the OpenRAC folder.
2. Get the file's SHA-256 in PowerShell:

   ```powershell
   Get-FileHash "C:\path\to\file.dll" -Algorithm SHA256
   ```

3. Report it on the vendor's false positive form. Give the file (or a download link if it is
   too big), the detection name, the SHA-256 and a short note, for example: "Open-source game
   port, GPL-3.0. Source: https://github.com/OpenRAC/OpenRAC".

Each build has a new SHA-256, so a new build can be flagged again. Report it again if so.

## Vendor forms

- Bitdefender: <https://www.bitdefender.com/submit/>
- Microsoft Defender: <https://www.microsoft.com/en-us/wdsi/filesubmission>
