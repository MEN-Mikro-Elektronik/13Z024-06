#***************************  M a k e f i l e  *******************************
#
#         Author: kp
#          $Date: 2006/03/01 18:58:46 $
#      $Revision: 1.3 $
#
#    Description: Makefile definitions for the MMODPRG driver (swapped variant)
#                 default: ADDRSPACE_SIZE = 0x100 bytes
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver_sw.mak,v $
#   Revision 1.3  2006/03/01 18:58:46  cs
#   ADDRSPACE_SIZE, MAC_BYTESWAP, ID_SW set here
#
#   Revision 1.2  2005/08/22 15:17:37  ub
#   unneccesary dependencies removed
#
#   Revision 1.1  2002/06/06 15:34:58  kp
#   Initial Revision
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2001 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=mmodprg_sw

MAK_SWITCH=$(SW_PREFIX)MAC_MEM_MAPPED \
           $(SW_PREFIX)MAC_BYTESWAP \
           $(SW_PREFIX)ID_SW \
           $(SW_PREFIX)MMODPRG_VARIANT=MMODPRG_SW \
           $(SW_PREFIX)MMODPRG_ADDRSPACE_SIZE=0x100


MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/id_sw$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)	\
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




