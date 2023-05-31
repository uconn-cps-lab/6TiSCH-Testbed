if exist .\lib-gen\lib-gen.ewd ( 
	FOR /f %%i in ('xml.exe sel -t -v "//configuration/name" .\lib-gen\lib-gen.ewd') DO (
		rmdir /s /q .\lib-gen\configPkg
		rmdir /s /q .\lib-gen\settings
		rmdir /s /q .\lib-gen\src
		iarbuild.exe ".\lib-gen\lib-gen.ewp" -clean %%i -log errors
		iarbuild.exe ".\lib-gen\lib-gen.ewp" -make %%i -log errors -parallel 4 -varfile .\lib-gen\lib-gen.custom_argvars
	)
)