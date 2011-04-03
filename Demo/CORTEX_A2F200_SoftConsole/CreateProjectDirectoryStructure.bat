REM This file should be executed from the command line prior to the first
REM build.  It will be necessary to refresh the Eclipse project once the
REM .bat file has been executed (normally just press F5 to refresh).

REM Copies all the required files from their location within the standard
REM FreeRTOS directory structure to under the Eclipse project directory.
REM This permits the Eclipse project to be used in 'managed' mode and without
REM having to setup any linked resources.

REM Have the files already been copied?
IF EXIST FreeRTOS_Source Goto END

	REM Create the required directory structure.
	MD FreeRTOS_Source
	MD FreeRTOS_Source\include	
	MD FreeRTOS_Source\portable\GCC
	MD FreeRTOS_Source\portable\GCC\ARM_CM3
	MD FreeRTOS_Source\portable\MemMang	
	MD FreeTCPIP
	MD FreeTCPIP\http_Common
	MD FreeTCPIP\apps
	MD FreeTCPIP\apps\httpd
	MD FreeTCPIP\net
	MD FreeTCPIP\sys
		
	REM Copy the core kernel files.
	copy ..\..\Source\tasks.c FreeRTOS_Source
	copy ..\..\Source\queue.c FreeRTOS_Source
	copy ..\..\Source\list.c FreeRTOS_Source
	copy ..\..\Source\timers.c FreeRTOS_Source
	
	REM Copy the common header files
	copy ..\..\Source\include\*.* FreeRTOS_Source\include
	
	REM Copy the portable layer files
	copy ..\..\Source\portable\GCC\ARM_CM3\*.* FreeRTOS_Source\portable\GCC\ARM_CM3
	
	REM Copy the basic memory allocation files
	copy ..\..\Source\portable\MemMang\heap_1.c FreeRTOS_Source\portable\MemMang
	
	REM Copy the core FreeTCPIP (based on uIP) files
	copy ..\Common\ethernet\FreeTCPIP\psock.c FreeTCPIP
	copy ..\Common\ethernet\FreeTCPIP\timer.c FreeTCPIP
	copy ..\Common\ethernet\FreeTCPIP\uip.c FreeTCPIP
	copy ..\Common\ethernet\FreeTCPIP\uip_arp.c FreeTCPIP
	
	REM Copy the FreeTCPIP (based on uIP) header files
	copy ..\Common\ethernet\FreeTCPIP\apps\httpd\*.h FreeTCPIP\apps\httpd
	copy ..\Common\ethernet\FreeTCPIP\net\*.h FreeTCPIP\net
	copy ..\Common\ethernet\FreeTCPIP\sys\*.h FreeTCPIP\sys
	
	
	REM Copy the core HTTPD files
	copy ..\Common\ethernet\FreeTCPIP\apps\httpd\http-strings.c FreeTCPIP\http_Common
	copy ..\Common\ethernet\FreeTCPIP\apps\httpd\httpd-fs.c FreeTCPIP\http_Common
	copy ..\Common\ethernet\FreeTCPIP\apps\httpd\httpd.c FreeTCPIP\http_Common	
	
: END
