rd /Q /S Project\MDK-ARM\Flash
del /Q Project\MDK-ARM\*.bak
del /Q Project\MDK-ARM\*.dep
del /Q Project\MDK-ARM\JLink*
del /Q Project\MDK-ARM\*.uvgui.* 
del /Q Project\MDK-ARM\*.uvguix.* 

del /Q Project\EWARM\*.dep
del /Q Project\EWARM\Flash
del /Q Project\EWARM\settings
rd /Q /S Project\EWARM\Flash
rd /Q /S Project\EWARM\settings


rd /Q /S Output\IAR
rd /Q /S Output\MDK
rd /Q /S Output



