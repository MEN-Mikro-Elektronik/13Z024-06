#************************** MDIS4 device descriptor *************************
#
#        Author: kp
#         $Date: 2002/06/06 15:34:59 $
#     $Revision: 1.1 $
#
#   Description: Metadescriptor for MMODPRG
#
#****************************************************************************

MMODPRG_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   MMODPRG       # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   D201_1      # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------
}
