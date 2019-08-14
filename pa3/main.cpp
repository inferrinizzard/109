// $Id: main.cpp,v 1.11 2018-01-25 14:19:29-08 - - $

#include <cstdlib>
#include <cassert>
#include <exception>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <unistd.h>

using namespace std;

#include "listmap.h"
#include "xpair.h"
#include "util.h"

using str_str_map = listmap<string, string>;
using str_str_pair = str_str_map::value_type;

void scan_options(int argc, char **argv)
{
   opterr = 0;
   for (;;)
   {
      int option = getopt(argc, argv, "@:");
      if (option == EOF)
         break;
      switch (option)
      {
      case '@':
         debugflags::setflags(optarg);
         break;
      default:
         complain() << "-" << char(optopt)
                    << ": invalid option" << endl;
         break;
      }
   }
}

int main(int argc, char **argv)
{
   sys_info::execname(argv[0]);
   scan_options(argc, argv);

   str_str_map inputs;
   for (char **argp = &argv[optind]; argp != &argv[argc]; ++argp)
   {
      str_str_pair pair(*argp, to_string<int>(argp - argv));
      inputs.insert(pair);
   }
   if (optind < 0 && argc == 1)
      inputs.insert(str_str_pair("-", "1"));

   regex octothorpe{R"(^\s*(#.*)?$)"};
   regex trim{R"(^\s*([^=]+?)\s*$)"};
   regex equalsSplit{R"(^\s*(.*?)\s*=\s*(.*?)\s*$)"};

   for (str_str_map::iterator itor = inputs.begin();
        itor != inputs.end(); ++itor)
   {
      str_str_map list;
      ifstream filein;
      if (itor->first != "-")
      {
         filein.open(itor->first);
         if (!filein)
         {
            cerr << "keyvalue: " << itor->first
                 << "No such file or directory" << endl;
            continue;
         }
      }

      int lineNum = 0;
      string in;
      while (!(itor->first == "-" ? cin.eof() : filein.eof()))
      {
         getline((itor->first == "-" ? cin : filein), in);
         lineNum++;
         smatch result;
         cout << itor->first << ": " << lineNum << ": " << in << endl;
         if (regex_search(in, result, octothorpe))
            continue;
         if (regex_search(in, result, equalsSplit))
            if (result[1].length() > 0)
               if (result[2].length() > 0)
               {
                  list.insert(xpair<const string, string>(
                      result[1], result[2]));
                  std::cout << result[1] << " = " << result[2] << endl;
               }
               else
                  list.erase(list.find(itor->first));
            else
            {
               if (result[2].length() > 0)
               {
                  for (auto printi : list)
                     if (printi.second == result[2])
                        std::cout << printi.first << " = "
                                  << printi.second << endl;
               }
               else
                  for (auto printi : list)
                     std::cout << printi.first << " = "
                               << printi.second << endl;
            }
         else if (regex_search(in, result, trim))
            if (list.find(result[1]) == list.end())
               std::cout << result[1] << ": key not found" << endl;
            else
               std::cout << list.find(result[1])->first << " = "
                         << list.find(result[1])->second << endl;
         else
            assert(false);
      }
      for (str_str_map::iterator litor = list.begin();
           litor != list.end(); litor = list.erase(litor))
         ;

      if (itor->first != "-")
         filein.close();
   }

   for (str_str_map::iterator itor = inputs.begin();
        itor != inputs.end(); itor = inputs.erase(itor))
      ;

   return EXIT_SUCCESS;
}
