#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/03/01 18:58:48 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the MMODPRG driver
#                 default: ADDRSPACE_SIZE = 0x1000 bytes
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver_4k.mak,v $
#   Revision 1.1  2006/03/01 18:58:48  cs
#   Initial Revision
#
#   Revision 1.2  2005/08/22 15:17:35  ub
#   unneccesary dependencies removed
#
#   Revision 1.1  2002/06/06 15:34:57  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2000 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=mmodprg_4k

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
           $(SW_PREFIX)MMODPRG_VARIANT=MMODPRG_4K \
           $(SW_PREFIX)MMODPRG_ADDRSPACE_SIZE=0x1000

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
		$(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)	\
		$(LIB_PREFIX)$(MEN_LIB_DIR)/id$(LIB_SUFFIX)	\
		$(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)	\


MAK_INCL=$(MEN_INC_DIR)/mmodprg_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/oss.h		\
         $(MEN_INC_DIR)/mdis_err.h	\
		 $(MEN_INC_DIR)/maccess.h	\
         $(MEN_INC_DIR)/desc.h		\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_com.h	\
         $(MEN_INC_DIR)/modcom.h	\
         $(MEN_INC_DIR)/ll_defs.h	\
         $(MEN_INC_DIR)/ll_entry.h	\
         $(MEN_INC_DIR)/dbg.h		\

MAK_INP1=mmodprg_drv$(INP_SUFFIX)
MAK_INP2=

MAK_INP=$(MAK_INP1) \
        $(MAK_INP2)
