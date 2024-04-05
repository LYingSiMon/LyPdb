# LyPdb
a Windows kernel Pdb parsing and downloading library that running purely in kernel mode without any R3 programs.

This project references https://github.com/Oxygen1a1/oxgenPdb, fixes some bugs causing blue screens, and stores symbol files to c:\lypdb\pdbname\pdbguid*.pdb to avoid blue screens due to outdated original symbols caused by system updates.
