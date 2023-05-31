FOR /D %%G in ("*") DO (
	if NOT "%%G"=="lib-gen" (
		if exist .\%%G\%%G.ewd ( 
			FOR /f %%i in ('xml.exe sel -t -v "//configuration/name" .\%%G\%%G.ewd') DO (
				rmdir /s /q .\%%G\configPkg
				rmdir /s /q .\%%G\settings
				rmdir /s /q .\%%G\src
				iarbuild.exe ".\%%G\%%G.ewp" -clean %%i -log errors
				iarbuild.exe ".\%%G\%%G.ewp" -make %%i -log errors -parallel 4 -varfile .\%%G\%%G.custom_argvars
			)
		)
	)
)