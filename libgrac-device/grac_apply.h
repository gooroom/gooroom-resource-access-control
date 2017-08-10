/*
 * grac_access_apply.h
 */

#ifndef _GRAC_ACCESS_APPLY_H_
#define _GRAC_ACCESS_APPLY_H_

#include <glib.h>

#include "grac_access.h"
#include "grac_apply_network.h"
#include "grac_apply_printer.h"
#include "grac_apply_usb.h"
#include "grac_apply_capture.h"
#include "grac_apply_dir.h"

// UID based access control
gboolean grac_apply_access_by_user(GracAccess *access, char *username);

#endif /* _GRM_ACCESS_APPLY_H_ */
