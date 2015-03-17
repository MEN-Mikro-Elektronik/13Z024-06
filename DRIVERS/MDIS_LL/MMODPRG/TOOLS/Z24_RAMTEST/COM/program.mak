#***************************  M a k e f i l e  *******************************
#
#         Author: ub/cs
#          $Date: 2006/02/24 17:27:16 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for z24 SRAM test tool
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2006/02/24 17:27:16  cs
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#  (c) Copyright 2006 by MEN Mikro Elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=z24_ramtest
MAK_SWITCH=

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)	\

MAK_INCL=$(MEN_INC_DIR)/mmodprg_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h	\
         $(MEN_INC_DIR)/usr_utl.h	\

MAK_INP1=z24_ramtest$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
