
const char COPYRIGHT[] =
          "Copyright (c) 1998-2000 Stephen Williams (steve@icarus.com)";

/*
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
#ident "$Id: main.cc,v 1.52 2002/04/04 05:26:13 steve Exp $"
#endif

# include "config.h"

const char NOTICE[] =
"  This program is free software; you can redistribute it and/or modify\n"
"  it under the terms of the GNU General Public License as published by\n"
"  the Free Software Foundation; either version 2 of the License, or\n"
"  (at your option) any later version.\n"
"\n"
"  This program is distributed in the hope that it will be useful,\n"
"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"  GNU General Public License for more details.\n"
"\n"
"  You should have received a copy of the GNU General Public License\n"
"  along with this program; if not, write to the Free Software\n"
"  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA\n"
;

# include  <stdio.h>
# include  <iostream.h>
# include  <fstream>
# include  <queue>
# include  <list>
# include  <map>
# include  <unistd.h>
# include  <stdlib.h>
#if defined(HAVE_TIMES)
# include  <sys/times.h>
#endif
#if defined(HAVE_GETOPT_H)
# include  <getopt.h>
#endif
# include  "pform.h"
# include  "parse_api.h"
# include  "netlist.h"
# include  "target.h"
# include  "compiler.h"

#if defined(__MINGW32__) && !defined(HAVE_GETOPT_H)
extern "C" int getopt(int argc, char*argv[], const char*fmt);
extern "C" int optind;
extern "C" const char*optarg;
#endif

#if defined(__CYGWIN32__) && !defined(HAVE_GETOPT_H)
extern "C" int getopt(int argc, char*argv[], const char*fmt);
extern "C" int optind;
extern "C" const char*optarg;
#endif

const char VERSION[] = "$Name:  $ $State: Exp $";

const char*target = "null";

map<string,string> flags;

list<const char*> library_dirs;
list<const char*> library_suff;

FILE *depend_file = NULL;
/*
 * These are the warning enable flags.
 */
bool warn_implicit = false;

/*
 * Verbose messages enabled.
 */
bool verbose_flag = false;


static void parm_to_flagmap(const string&flag)
{
      string key, value;
      unsigned off = flag.find('=');
      if (off > flag.size()) {
	    key = flag;
	    value = "";

      } else {
	    key = flag.substr(0, off);
	    value = flag.substr(off+1);
      }

      flags[key] = value;
}


extern Design* elaborate(list <const char*>root);

extern void cprop(Design*des);
extern void synth(Design*des);
extern void syn_rules(Design*des);
extern void nodangle(Design*des);
extern void xnfio(Design*des);

typedef void (*net_func)(Design*);
static struct net_func_map {
      const char*name;
      void (*func)(Design*);
} func_table[] = {
      { "cprop",   &cprop },
      { "nodangle",&nodangle },
      { "synth",   &synth },
      { "syn-rules",   &syn_rules },
      { "xnfio",   &xnfio },
      { 0, 0 }
};

net_func name_to_net_func(const string&name)
{
      for (unsigned idx = 0 ;  func_table[idx].name ;  idx += 1)
	    if (name == func_table[idx].name)
		  return func_table[idx].func;

      return 0;
}

const char *net_func_to_name(const net_func func)
{
      for (unsigned idx = 0 ;  func_table[idx].name ;  idx += 1)
	    if (func == func_table[idx].func)
		  return func_table[idx].name;

      return "This cannot happen";
}

#if defined(HAVE_TIMES)
static double cycles_diff(struct tms *a, struct tms *b)
{
      clock_t aa = a->tms_utime 
	    +      a->tms_stime 
	    +      a->tms_cutime 
	    +      a->tms_cstime;

      clock_t bb = b->tms_utime 
	    +      b->tms_stime 
	    +      b->tms_cutime 
	    +      b->tms_cstime;

      return (aa-bb)/(double)sysconf(_SC_CLK_TCK);
}
#else // ! defined(HAVE_TIMES)
// Provide dummies
struct tms { int x; };
inline static void times(struct tms *) { }
inline static double cycles_diff(struct tms *a, struct tms *b) { return 0; }
#endif // ! defined(HAVE_TIMES)

int main(int argc, char*argv[])
{
      bool help_flag = false;
      bool times_flag = false;

      const char* net_path = 0;
      const char* pf_path = 0;
      const char* warn_en = "";
      int opt;
      unsigned flag_errors = 0;
      queue<net_func> net_func_queue;
      list<const char*> roots;
      const char* depfile_name = NULL;

      struct tms cycles[5];

      flags["VPI_MODULE_LIST"] = "system";
      flags["-o"] = "a.out";
      min_typ_max_flag = TYP;
      min_typ_max_warn = 10;

      while ((opt = getopt(argc, argv, "F:f:hm:M:N:o:P:p:s:T:t:VvW:Y:y:")) != EOF) switch (opt) {
	  case 'F': {
		net_func tmp = name_to_net_func(optarg);
		if (tmp == 0) {
		      cerr << "No such design transform function ``"
			   << optarg << "''." << endl;
		      flag_errors += 1;
		      break;
		}
		net_func_queue.push(tmp);
		break;
	  }
	  case 'f':
	    parm_to_flagmap(optarg);
	    break;
	  case 'h':
	    help_flag = true;
	    break;
	  case 'm':
	    flags["VPI_MODULE_LIST"] = flags["VPI_MODULE_LIST"]+","+optarg;
	    break;
	  case 'M':
	    depfile_name = optarg;
	    break;
	  case 'N':
	    net_path = optarg;
	    break;
	  case 'o':
	    flags["-o"] = optarg;
	    break;
	  case 'P':
	    pf_path = optarg;
	    break;
	  case 'p':
	    parm_to_flagmap(optarg);
	    break;
	  case 's':
	    roots.push_back(optarg);
	    break;
	  case 'T':
	    if (strcmp(optarg,"min") == 0) {
		  min_typ_max_flag = MIN;
		  min_typ_max_warn = 0;
	    } else if (strcmp(optarg,"typ") == 0) {
		  min_typ_max_flag = TYP;
		  min_typ_max_warn = 0;
	    } else if (strcmp(optarg,"max") == 0) {
		  min_typ_max_flag = MAX;
		  min_typ_max_warn = 0;
	    } else {
		  cerr << "Invalid argument (" << optarg << ") to -T flag."
		       << endl;
		  flag_errors += 1;
	    }
	    break;
	  case 't':
	    target = optarg;
	    break;
	  case 'v':
	    verbose_flag = true;
#          if defined(HAVE_TIMES)
	    times_flag = true;
#          endif
	    break;
	  case 'V':
	    cout << "Icarus Verilog version " << VERSION << endl;
	    cout << COPYRIGHT << endl;
	    cout << endl << NOTICE << endl;
	    return 0;
	  case 'W':
	    warn_en = optarg;
	    break;
	  case 'y':
	    library_dirs.push_back(optarg);
	    break;
	  case 'Y':
	    library_suff.push_back(optarg);
	    break;
	  default:
	    flag_errors += 1;
	    break;
      }

      if (flag_errors)
	    return flag_errors;

      if (help_flag) {
	    cout << "Icarus Verilog version " << VERSION << endl <<
"usage: ivl <options> <file>\n"
"options:\n"
"\t-F <name>        Apply netlist function <name>.\n"
"\t-h               Print usage information, and exit.\n"
"\t-m <module>      Load vpi module <module>.\n"
"\t-N <file>        Dump the elaborated netlist to <file>.\n"
"\t-o <file>        Write output to <file>.\n"
"\t-P <file>        Write the parsed input to <file>.\n"
"\t-p <assign>      Set a parameter value.\n"
"\t-s <module>      Select the top-level module.\n"
"\t-T [min|typ|max] Select timing corner.\n"
"\t-t <name>        Select target <name>.\n"
"\t-v               Print progress indications"
#if defined(HAVE_TIMES)
                                           " and execution times"
#endif
                                           ".\n"
"\t-V               Print version and copyright information, and exit.\n"
"\t-y <dir>         Add directory to library search path.\n"
"\t-Y <suf>         Add suffix string library search path.\n"

		  ;
	    cout << "Netlist functions:" << endl;
	    for (unsigned idx = 0 ;  func_table[idx].name ;  idx += 1)
		  cout << "\t-F " << func_table[idx].name << endl;
	    cout << "Target types:" << endl;
	    for (unsigned idx = 0 ;  target_table[idx] ;  idx += 1)
		  cout << "\t-t " << target_table[idx]->name << endl;
	    return 0;
      }

      if (optind == argc) {
	    cerr << "No input files." << endl;
	    return 1;
      }

      if( depfile_name ) {
	      depend_file = fopen(depfile_name, "a");
	      if(! depend_file) {
		      perror(depfile_name);
	      }
      }
	      

	/* If there were no -Y flags, then create a minimal library
	   suffix search list. */
      if (library_suff.empty()) {
	    library_suff.push_back(".v");
      }

	/* Scan the warnings enable string for warning flags. */
      for (const char*cp = warn_en ;  *cp ;  cp += 1) switch (*cp) {
	  case 'i':
	    warn_implicit = true;
	    break;
	  default:
	    break;
      }

      if (verbose_flag) {
	    if (times_flag)
		  times(cycles+0);
	    cout << "PARSING INPUT ..." << endl;
      }

	/* Parse the input. Make the pform. */
      int rc = pform_parse(argv[optind]);

      if (pf_path) {
	    ofstream out (pf_path);
	    out << "PFORM DUMP MODULES:" << endl;
	    for (map<string,Module*>::iterator mod = pform_modules.begin()
		       ; mod != pform_modules.end()
		       ; mod ++ ) {
		  pform_dump(out, (*mod).second);
	    }
	    out << "PFORM DUMP PRIMITIVES:" << endl;
	    for (map<string,PUdp*>::iterator idx = pform_primitives.begin()
		       ; idx != pform_primitives.end()
		       ; idx ++ ) {
		  (*idx).second->dump(out);
	    }
      }

      if (rc) {
	    return rc;
      }


	/* If the user did not give specific module(s) to start with,
	   then look for modules that are not instantiated anywhere.  */

      if (roots.empty()) {
	    map<string,bool> mentioned_p;
	    map<string,Module*>::iterator mod;
	    if (verbose_flag)
		  cout << "LOCATING TOP-LEVEL MODULES..." << endl << "  ";
	    for (mod = pform_modules.begin()
		       ; mod != pform_modules.end()
		       ; mod++) {
		  list<PGate*> gates = (*mod).second->get_gates();
		  list<PGate*>::const_iterator gate;
		  for (gate = gates.begin(); gate != gates.end(); gate++) {
			PGModule *mod = dynamic_cast<PGModule*>(*gate);
			if (mod) {
			      // Note that this module has been instantiated
			      mentioned_p[mod->get_type()] = true;
			}
		  }
	    }

	    for (mod = pform_modules.begin()
		       ; mod != pform_modules.end()
		       ; mod++) {
		  if (mentioned_p[(*mod).second->mod_name()] == false) {
			if (verbose_flag)
			      cout << " " << (*mod).second->mod_name();
			roots.push_back((*mod).second->mod_name());
		  }
	    }
	    if (verbose_flag)
		  cout << endl;
      }
	    
	/* If there is *still* no guess for the root module, then give
	   up completely, and complain. */

      if (roots.empty()) {
	    cerr << "No top level modules, and no -s option." << endl;
	    return 1;
      }


      if (verbose_flag) {
	    if (times_flag) {
		  times(cycles+1);
		  cerr<<" ... done, "
		      <<cycles_diff(cycles+1, cycles+0)<<" seconds."<<endl;
	    }
	    cout << "ELABORATING DESIGN" << endl;
      }

	/* On with the process of elaborating the module. */
      Design*des = elaborate(roots);
      if (des == 0) {
	    cerr << "Elaboration failed" << endl;
	    return 1;
      }

      if (des->errors) {
	    cerr << des->errors
		 << " error(s) during elaboration." << endl;
	    return des->errors;
      }

      des->set_flags(flags);


      if (verbose_flag) {
	    if (times_flag) {
		  times(cycles+2);
		  cerr<<" ... done, "
		      <<cycles_diff(cycles+2, cycles+1)<<" seconds."<<endl;
	    }
	    cout << "RUNNING FUNCTORS ..." << endl;
      }

      while (!net_func_queue.empty()) {
	    net_func func = net_func_queue.front();
	    net_func_queue.pop();
	    if (verbose_flag)
		  cerr<<" -F "<<net_func_to_name(func)<<endl;
	    func(des);
      }

      if (net_path) {
	    ofstream out (net_path);
	    des->dump(out);
      }


      if (verbose_flag) {
	    if (times_flag) {
		  times(cycles+3);
		  cerr<<" ... done, "
		      <<cycles_diff(cycles+3, cycles+2)<<" seconds."<<endl;
	    }
	    cout << "CODE GENERATION -t "<<target<<" ..." << endl;
      }

      bool emit_rc = emit(des, target);
      if (!emit_rc) {
	    cerr << "error: Code generation had errors." << endl;
	    return 1;
      }

      if (verbose_flag) {
	    if (times_flag) {
		  times(cycles+4);
		  cerr<<" ... done, "
		      <<cycles_diff(cycles+4, cycles+3)<<" seconds."<<endl;
	    } else
		  cout << "DONE." << endl;
      }

      return 0;
}

/*
 * $Log: main.cc,v $
 * Revision 1.52  2002/04/04 05:26:13  steve
 *  Add dependency generation.
 *
 * Revision 1.51  2001/11/16 05:07:19  steve
 *  Add support for +libext+ in command files.
 *
 * Revision 1.50  2001/10/20 23:02:40  steve
 *  Add automatic module libraries.
 *
 * Revision 1.49  2001/10/20 05:21:51  steve
 *  Scope/module names are char* instead of string.
 *
 * Revision 1.48  2001/10/19 21:53:24  steve
 *  Support multiple root modules (Philip Blundell)
 *
 * Revision 1.47  2001/07/30 02:44:05  steve
 *  Cleanup defines and types for mingw compile.
 *
 * Revision 1.46  2001/07/25 03:10:49  steve
 *  Create a config.h.in file to hold all the config
 *  junk, and support gcc 3.0. (Stephan Boettcher)
 *
 * Revision 1.45  2001/07/16 18:14:56  steve
 *  Reshuffle -v and -V flags of ivl. (Stephan Boettcher)
 *
 * Revision 1.44  2001/07/03 04:09:24  steve
 *  Generate verbuse status messages (Stephan Boettcher)
 *
 * Revision 1.43  2001/07/02 01:57:27  steve
 *  Add the -V flag, and some verbose messages.
 *
 * Revision 1.42  2001/06/23 18:41:02  steve
 *  Include stdlib.h
 *
 * Revision 1.41  2001/05/20 17:35:05  steve
 *  declare getopt by hand in mingw32 compile.
 *
 * Revision 1.40  2001/01/20 19:02:05  steve
 *  Switch hte -f flag to the -p flag.
 *
 * Revision 1.39  2000/11/22 20:48:32  steve
 *  Allow sole module to be a root.
 *
 * Revision 1.38  2000/09/12 01:17:40  steve
 *  Version information for vlog_vpi_info.
 *
 * Revision 1.37  2000/08/09 03:43:45  steve
 *  Move all file manipulation out of target class.
 *
 * Revision 1.36  2000/07/29 17:58:21  steve
 *  Introduce min:typ:max support.
 *
 * Revision 1.35  2000/07/14 06:12:57  steve
 *  Move inital value handling from NetNet to Nexus
 *  objects. This allows better propogation of inital
 *  values.
 *
 *  Clean up constant propagation  a bit to account
 *  for regs that are not really values.
 *
 * Revision 1.34  2000/05/13 20:55:47  steve
 *  Use yacc based synthesizer.
 *
 * Revision 1.33  2000/05/08 05:29:43  steve
 *  no need for nobufz functor.
 *
 * Revision 1.32  2000/05/03 22:14:31  steve
 *  More features of ivl available through iverilog.
 *
 * Revision 1.31  2000/04/12 20:02:53  steve
 *  Finally remove the NetNEvent and NetPEvent classes,
 *  Get synthesis working with the NetEvWait class,
 *  and get started supporting multiple events in a
 *  wait in vvm.
 */

