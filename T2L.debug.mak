###########################################################
# Makefile generated by xIDE for uEnergy                   
#                                                          
# Project: T2L
# Configuration: Debug
# Generated: Mon Jun 20 16:01:18 2016
#                                                          
# WARNING: Do not edit this file. Any changes will be lost 
#          when the project is rebuilt.                    
#                                                          
###########################################################

XIDE_PROJECT=T2L
XIDE_CONFIG=Debug
OUTPUT=T2L
OUTDIR=C:/Isaac/Toys2Life/T2LFW
DEFS=

OUTPUT_TYPE=0
USE_FLASH=0
ERASE_NVM=1
CSFILE_CSR101x_A05=C:\Isaac\Toys2Life\T2LFW\277749.keyr
MASTER_DB=
LIBPATHS=
INCPATHS=
STRIP_SYMBOLS=0
OTAU_BOOTLOADER=0
OTAU_CSFILE=
OTAU_NAME=
OTAU_SECRET=
OTAU_VERSION=7

DBS=


INPUTS=\
      app_debug.c\
      app_main.c\
      $(DBS)

KEYR=

# Project-specific options
csr100x_keyr=

-include T2L.mak
include $(SDK)/genmakefile.uenergy