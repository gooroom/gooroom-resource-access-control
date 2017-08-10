/*
 * sys_acl.h
 *
 *  Created on: 2016. 1. 20.
 *      Author: user
 */

/* todo
 *      get list all rules
 */


#ifndef _SYS_ACL_H_
#define _SYS_ACL_H_

#include <sys/acl.h>
#include <acl/libacl.h>
#include <glib.h>

// 2016.5
gboolean sys_acl_clear(gchar *path);	// all ACL
gboolean sys_acl_clear_by_type(gchar *path, acl_type_t type);  // ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS

void sys_acl_set_recalc_mask(gboolean recalcB);		// 2016.5  default mode = TRUE


// type
//			ACL_TYPE_DEFAULT, ACL_TYPE_ACCESS
// tag
//		 ACL_USER_OBJ	   ACL_GROUP_OBJ   --> file owner
//	   ACL_USER	       ACL_GROUP       --> additional
//     ACL_MASK
//     ACL_OTHER		(0x20)
// permission (can be oring)
//		ACL_READ, ACL_WRITE, ACL_EXECUTE

//  id or name is valid only for ACL_USER or ACL_GROUP
gboolean sys_acl_set_by_id(gchar *path, id_t id, acl_type_t type, acl_tag_t tag, acl_perm_t perm);
gboolean sys_acl_get_by_id(gchar *path, id_t id, acl_type_t type, acl_tag_t tag, acl_perm_t *perm);
gboolean sys_acl_del_by_id(gchar *path, id_t id, acl_type_t type, acl_tag_t tag);

gboolean sys_acl_set_by_name(gchar *path, gchar *name, acl_type_t type, acl_tag_t tag, acl_perm_t perm);
gboolean sys_acl_get_by_name(gchar *path, gchar *name, acl_type_t type, acl_tag_t tag, acl_perm_t *perm);
gboolean sys_acl_del_by_name(gchar *path, gchar *name, acl_type_t type, acl_tag_t tag);


// Do noy use bellow functions
// old method : maybe deleted
// permission : same as standard linux command "chmod" (ex) "rwx"

gboolean sys_acl_set_user (gchar *path, gchar*user, gchar* perm);
gboolean sys_acl_set_uid  (gchar *path, uid_t uid,  gchar* perm);
gboolean sys_acl_set_group(gchar *path, gchar*group, gchar* perm);
gboolean sys_acl_set_gid  (gchar *path, uid_t gid,   gchar* perm);
gboolean sys_acl_set_other(gchar *path, gchar* perm);
gboolean sys_acl_set_mask(gchar *path, gchar* perm);

gboolean sys_acl_default_set_user (gchar *path, gchar*user, gchar* perm);
gboolean sys_acl_default_set_uid  (gchar *path, uid_t uid,  gchar* perm);
gboolean sys_acl_default_set_group(gchar *path, gchar*group, gchar* perm);
gboolean sys_acl_default_set_gid  (gchar *path, uid_t gid,   gchar* perm);
gboolean sys_acl_default_set_other(gchar *path, gchar* perm);
gboolean sys_acl_default_set_mask(gchar *path, gchar* perm);

gboolean sys_acl_del_user (gchar *path, gchar*user);
gboolean sys_acl_del_uid  (gchar *path, uid_t uid);
gboolean sys_acl_del_group(gchar *path, gchar*group);
gboolean sys_acl_del_gid  (gchar *path, uid_t gid);
//gboolean sys_acl_del_other(gchar *path);		//   no meaning
gboolean sys_acl_del_mask(gchar *path);

gboolean sys_acl_default_del_user (gchar *path, gchar*user);
gboolean sys_acl_default_del_uid  (gchar *path, uid_t uid);
gboolean sys_acl_default_del_group(gchar *path, gchar*group);
gboolean sys_acl_default_del_gid  (gchar *path, uid_t gid);
gboolean sys_acl_default_del_other(gchar *path);
gboolean sys_acl_default_del_mask(gchar *path);


gboolean sys_acl_get_user (gchar *path, gchar*user, gchar* perm);
gboolean sys_acl_get_uid  (gchar *path, uid_t uid,  gchar* perm);
gboolean sys_acl_get_group(gchar *path, gchar*group, gchar* perm);
gboolean sys_acl_get_gid  (gchar *path, uid_t gid,   gchar* perm);
gboolean sys_acl_get_other(gchar *path, gchar* perm);
gboolean sys_acl_get_mask (gchar *path, gchar* perm);

gboolean sys_acl_default_get_user (gchar *path, gchar*user, gchar* perm);
gboolean sys_acl_default_get_uid  (gchar *path, uid_t uid,  gchar* perm);
gboolean sys_acl_default_get_group(gchar *path, gchar*group, gchar* perm);
gboolean sys_acl_default_get_gid  (gchar *path, uid_t gid,   gchar* perm);
gboolean sys_acl_default_get_other(gchar *path, gchar* perm);
gboolean sys_acl_default_get_mask(gchar *path, gchar* perm);


#endif /* SYS_ACL_H_ */
