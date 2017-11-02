/*
 * grac_printer.h
 *
 *  Created on: 2017. 10. 30.
 *      Author: yang
 */

#ifndef _GRAC_PRINTER_H_
#define _GRAC_PRINTER_H_

#include <glib.h>

gboolean grac_printer_init();
gboolean grac_printer_apply(gboolean allow);
gboolean grac_printer_end();


#endif /* _GRAC_PRINTER_H_ */
