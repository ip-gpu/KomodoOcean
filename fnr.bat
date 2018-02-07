@echo off
set datapath=%~dp0
IF %datapath:~-1%==\ SET datapath=%datapath:~0,-1%
fnr --cl --dir "%datapath%" --fileMask "Makefile.Release" --useRegEx --find " -DNDEBUG" --replace ""
