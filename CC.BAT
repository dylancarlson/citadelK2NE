@echo off
if "%1" == "" goto error
if "%1" == "citadel" goto ctdl
if "%1" == "confg" goto cnfg
if "%1" == "both" goto ctdl
:ctdl
make -fmakectdl.vaq
if "%1" == "both" goto cnfg
goto end
:cnfg
make -fmakecnfg.vaq
goto end
:error
echo PARAMETER bad or missing.
echo SYNTAX: CC [ctdl or cnfg]
:end
if "%1" == "both" goto exit
dir %1.exe
dir c:\c\%1.exe
pklite %1.exe
:exit

