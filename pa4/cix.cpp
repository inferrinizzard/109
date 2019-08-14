// $Id: cix.cpp,v 1.9 2019-04-05 15:04:28-07 - - $

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"
#include "logstream.h"
#include "sockets.h"

logstream outlog(cout);
struct cix_exit : public exception
{
};

unordered_map<string, cix_command> command_map{
    {"exit", cix_command::EXIT},
    {"help", cix_command::HELP},
    {"ls", cix_command::LS},
    {"get", cix_command::GET},
    {"put", cix_command::PUT},
    {"rm", cix_command::RM},
};

static const char help[] = R"||(
exit         - Exit the program.  Equivalent to EOF.
get filename - Copy remote file to local host.
help         - Print help summary.
ls           - List names of files on remote server.
put filename - Copy local file to remote host.
rm filename  - Remove file from remote server.
)||";

void cix_help()
{
   cout << help;
}

void cix_ls(client_socket &server)
{
   cix_header header{0, cix_command::LS};
   outlog << "sending header " << header << endl;
   send_packet(server, &header, sizeof header);
   recv_packet(server, &header, sizeof header);
   outlog << "received header " << header << endl;
   if (header.command != cix_command::LSOUT)
   {
      outlog << "sent LS, server did not return LSOUT" << endl;
      outlog << "server returned " << header << endl;
   }
   else
   {
      auto buffer = make_unique<char[]>(header.nbytes + 1);
      recv_packet(server, buffer.get(), header.nbytes);
      outlog << "received " << header.nbytes << " bytes" << endl;
      buffer[header.nbytes] = '\0';
      cout << buffer.get();
   }
}

void cix_get(client_socket &server, string file)
{
   cix_header header{0, cix_command::GET};
   strcpy(header.filename, file.c_str());
   outlog << "sending header " << header << endl;
   send_packet(server, &header, sizeof header);
   recv_packet(server, &header, sizeof header);
   outlog << "received header " << header << endl;
   if (header.command != cix_command::FILEOUT)
   {
      outlog << "sent GET, server did not return FILEOUT" << endl;
      outlog << "server returned " << header << endl;
      if (header.command == cix_command::NAK)
         outlog << "server error: " << strerror(header.nbytes) << endl;
   }
   else
   {
      auto buffer = make_unique<char[]>(header.nbytes + 1);
      recv_packet(server, buffer.get(), header.nbytes);
      outlog << "received " << header.nbytes << " bytes" << endl;
      buffer[header.nbytes] = '\0';
      ofstream get_out;
      get_out.open(header.filename);
      get_out << buffer.get();
      get_out.close();
   }
}

void cix_put(client_socket &server, string file)
{
   ifstream put_in;
   put_in.open(file, std::ifstream::in | std::ifstream::binary);
   if (put_in.bad())
   {
      outlog << file << ": No such file or directory" << endl;
      return;
   }
   stringstream in_data;
   while (put_in >> in_data.rdbuf())
      ;
   cix_header header{0, cix_command::PUT};
   strcpy(header.filename, file.c_str());
   header.nbytes = in_data.str().length();
   outlog << "sending header " << header << endl;
   send_packet(server, &header, sizeof header);
   send_packet(server, in_data.str().c_str(), in_data.str().length());
   outlog << "sent " << in_data.str().length() << " bytes" << endl;

   recv_packet(server, &header, sizeof header);
   outlog << "received header " << header << endl;
   if (header.command == cix_command::ACK)
      outlog << "put " + file + ": OK" << endl;
   else
   {
      outlog << "sent PUT, server did not return ACK" << endl;
      outlog << "server returned " << header << endl;
      if (header.command == cix_command::NAK)
         outlog << "server error: " << strerror(header.nbytes) << endl;
   }
}

void cix_rm(client_socket &server, string file)
{
   cix_header header{0, cix_command::RM};
   strcpy(header.filename, file.c_str());
   outlog << "sending header " << header << endl;
   send_packet(server, &header, sizeof header);
   recv_packet(server, &header, sizeof header);
   outlog << "received header " << header << endl;
   if (header.command != cix_command::ACK)
   {
      outlog << "sent RM, server did not return ACK" << endl;
      outlog << "server returned " << header << endl;
      if (header.command == cix_command::NAK)
         outlog << "server error: " << strerror(header.nbytes) << endl;
   }
   else
      outlog << "rm " + file + ": OK" << endl;
}

void usage()
{
   cerr << "Usage: " << outlog.execname() << " [host] [port]" << endl;
   throw cix_exit();
}

int main(int argc, char **argv)
{
   outlog.execname(basename(argv[0]));
   outlog << "starting" << endl;
   vector<string> args(&argv[1], &argv[argc]);
   if (args.size() > 2)
      usage();
   string host = get_cix_server_host(args, 0);
   in_port_t port = get_cix_server_port(args, 1);
   outlog << to_string(hostinfo()) << endl;
   try
   {
      outlog << "connecting to " << host << " port " << port << endl;
      client_socket server(host, port);
      outlog << "connected to " << to_string(server) << endl;
      for (;;)
      {
         string line;
         getline(cin, line);
         if (cin.eof())
            throw cix_exit();
         if (line == "")
            continue;
         outlog << "command " << line << endl;
         stringstream linestream(line);
         vector<string> toks;
         for (string tok; getline(linestream, tok, ' ');)
            toks.push_back(tok);
         const auto &itor = command_map.find(toks[0]);
         cix_command cmd = itor == command_map.end()
                               ? cix_command::ERROR
                               : itor->second;
         switch (cmd)
         {
         case cix_command::EXIT:
            throw cix_exit();
            break;
         case cix_command::HELP:
            cix_help();
            break;
         case cix_command::LS:
            cix_ls(server);
            break;
         case cix_command::RM:
            if (toks.size() == 1 || toks[1].length() > 58)
            {
               outlog << "Usage: rm <filename>" << endl;
               break;
            }
            cix_rm(server, toks[1]);
            break;
         case cix_command::GET:
            if (toks.size() == 1 || toks[1].length() > 58)
            {
               outlog << "Usage: get <filename>" << endl;
               break;
            }
            cix_get(server, toks[1]);
            break;
         case cix_command::PUT:
            if (toks.size() == 1 || toks[1].length() > 58)
            {
               outlog << "Usage: get <filename>" << endl;
               break;
            }
            cix_put(server, toks[1]);
            break;
         default:
            outlog << line << ": invalid command" << endl;
            break;
         }
      }
   }
   catch (socket_error &error)
   {
      outlog << error.what() << endl;
   }
   catch (cix_exit &error)
   {
      outlog << "caught cix_exit" << endl;
   }
   outlog << "finishing" << endl;
   return 0;
}
