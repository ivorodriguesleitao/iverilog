/*
 * Copyright (c) 2000 Stephen Williams (steve@picturel.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT) && !defined(macintosh)
#ident "$Id: build_string.c,v 1.7 2002/04/04 05:26:13 steve Exp $"
#endif

# include "config.h"

# include  "globals.h"
# include  <string.h>
# include  <assert.h>

int build_string(char*output, size_t olen, const char*pattern)
{
      char tmp_buf[1024];

      char*output_save = output;
      while (*pattern) {

	    if (*pattern == '%') {
		  pattern += 1;
		  switch (*pattern) {
		      case 0:
			break;

		      case '%':
			*output++ = '%';
			break;

		      case '[': {
			    const char*tail;
			    pattern += 1;
			    assert(*pattern);
			    tail = strchr(pattern+1, ']');
			    assert(tail);
			    strncpy(tmp_buf, pattern+1, tail-pattern-1);
			    tmp_buf[tail-pattern-1] = 0;

			    if (((*pattern == 's') && start)
				|| ((*pattern == 'v') && verbose_flag)
				|| ((*pattern == 'M') && depfile)
				|| ((*pattern == 'N') && npath)
				|| ((*pattern == 'T') && mtm)) {
				  int rc = build_string(output, olen,
							tmp_buf);
				  output += rc;
				  olen -= rc;
			    }
			    pattern = tail;
			    break;
		      }

		      case 'B':
			strcpy(output, base);
			output += strlen(base);
			olen -= strlen(base);
			break;

		      case 'f':
			if (f_list) {
			      strcpy(output, f_list);
			      output += strlen(f_list);
			      olen -= strlen(f_list);
			}
			break;

		      case 'm':
			if (mod_list) {
			      strcpy(output, mod_list);
			      output += strlen(mod_list);
			      olen -= strlen(mod_list);
			}
			break;

		      case 'M':
			if (depfile) {
			      strcpy(output, depfile);
			      output += strlen(depfile);
			      olen -= strlen(depfile);
			}
			break;

		      case 'N':
			if (npath) {
			      strcpy(output, npath);
			      output += strlen(npath);
			      olen -= strlen(npath);
			}
			break;

		      case 'o':
			strcpy(output, opath);
			output += strlen(opath);
			olen -= strlen(opath);
			break;

		      case 's':
			if (start) {
			      strcpy(output, start);
			      output += strlen(start);
			      olen -= strlen(start);
			}
			break;

		      case 'T':
			if (mtm) {
			      strcpy(output, mtm);
			      output += strlen(mtm);
			      olen -= strlen(mtm);
			}
			break;

		      case 't':
			strcpy(output, targ);
			output += strlen(targ);
			olen -= strlen(targ);
			break;

		      case 'W':
			strcpy(output, warning_flags);
			output += strlen(warning_flags);
			olen -= strlen(warning_flags);
			break;

		      case 'y':
			if (library_flags) {
			      strcpy(output, library_flags);
			      output += strlen(library_flags);
			      olen -= strlen(library_flags);
			}
			break;

		      case 'Y':
			if (library_flags2) {
			      strcpy(output, library_flags2);
			      output += strlen(library_flags2);
			      olen -= strlen(library_flags2);
			}
			break;

		  }
		  pattern += 1;

	    } else {
		  *output++ = *pattern++;
		  olen -= 1;
	    }
      }

      *output = 0;
      return output-output_save;
}

/*
 * $Log: build_string.c,v $
 * Revision 1.7  2002/04/04 05:26:13  steve
 *  Add dependency generation.
 *
 * Revision 1.6  2001/11/16 05:07:19  steve
 *  Add support for +libext+ in command files.
 *
 * Revision 1.5  2001/10/20 23:02:40  steve
 *  Add automatic module libraries.
 *
 * Revision 1.4  2001/07/25 03:10:50  steve
 *  Create a config.h.in file to hold all the config
 *  junk, and support gcc 3.0. (Stephan Boettcher)
 *
 * Revision 1.3  2001/07/03 04:09:25  steve
 *  Generate verbuse status messages (Stephan Boettcher)
 *
 * Revision 1.2  2000/10/28 03:45:47  steve
 *  Use the conf file to generate the vvm ivl string.
 *
 * Revision 1.1  2000/10/08 22:36:56  steve
 *  iverilog with an iverilog.conf configuration file.
 *
 */

