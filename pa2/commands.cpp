// $Id: commands.cpp,v 1.17 2018-01-25 14:02:55-08 - - $

#include "commands.h"
#include "debug.h"
#include <sstream>
#include <iomanip>

command_hash cmd_hash{
    {"cat", fn_cat},
    {"cd", fn_cd},
    {"echo", fn_echo},
    {"exit", fn_exit},
    {"ls", fn_ls},
    {"lsr", fn_lsr},
    {"make", fn_make},
    {"mkdir", fn_mkdir},
    {"prompt", fn_prompt},
    {"pwd", fn_pwd},
    {"rm", fn_rm},
    {"rmr", fn_rmr}};

command_fn find_command_fn(const string &cmd)
{
   // Note: value_type is pair<const key_type, mapped_type>
   // So: iterator->first is key_type (string)
   // So: iterator->second is mapped_type (command_fn)

   DEBUGF('c', "[" << cmd << "]");
   const auto result = cmd_hash.find(cmd);
   if (result == cmd_hash.end())
   {
      throw command_error(cmd + ": no such function");
   }
   return result->second;
}

command_error::command_error(const string &what)
    : runtime_error(what) {}

int exit_status_message()
{
   int exit_status = exit_status::get();
   cout << execname() << ": exit(" << exit_status << ")" << endl;
   return exit_status;
}

void fn_cat(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);

   if (words.size() == static_cast<size_t>(1))
      return;

   wordvec oldpath = vector<string>(*state.path());
   wordvec dirs = split(words[1], "/");
   string c = words[1];
   if (dirs.size() > 0)
   {
      c = dirs.back();
      dirs.pop_back();
      stringstream prebuff;
      prebuff << ".";
      for (string iter : dirs)
         prebuff << "/" << iter;
      fn_cd(state, vector<string>{"cd", prebuff.str()});
   }
   if (state.files().count(c) == 0)
      throw file_error("cat: " + c + ": No such file or directory");
   if (state.files()[c].get()->type() == file_type::DIRECTORY_TYPE)
      throw file_error("cd: " + c + ": is a directory");
   cout << state.files()[c].get()->file()->readfile() << endl;

   if (dirs.size() > 0)
   {
      stringstream buffer;
      for (string iter : oldpath)
         buffer << iter << "/";
      fn_cd(state, vector<string>{"cd"});
      fn_cd(state, vector<string>{"cd", buffer.str()});
   }
}

void fn_cd(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() == static_cast<size_t>(1) ||
       words[1] == "" || words[1] == "/")
   {
      state.path()->clear();
      state.set(state.top());
      return;
   }
   if (words[1] == ".")
      return;
   wordvec dirs = split(words[1], "/");
   dir files = state.files();
   if (files.count(dirs[0]) == 0)
      throw file_error("cd: " + dirs[0] +
                       ": No such file or directory");
   if (files[dirs[0]].get()->type() == file_type::PLAIN_TYPE)
      throw file_error("cd: " + dirs[0] + ": Is a file");
   state.set(files[dirs[0]]);
   if (dirs[0] == "..")
      state.path()->pop_back();
   else
      state.path()->push_back(dirs[0]);
   dirs.erase(dirs.begin());
   if (dirs.size() > 0)
   {
      stringstream buffer;
      for (string iter : dirs)
         buffer << iter << "/";
      string newpath = buffer.str();
      newpath.pop_back();
      fn_cd(state, vector<string>{words[0], newpath});
   }
}

void fn_echo(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() == static_cast<size_t>(1))
      return;
   cout << word_range(words.cbegin() + 1, words.cend()) << endl;
}

void fn_exit(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() > 1)
      try
      {
         exit_status::set(stoi(words[1]));
      }
      catch (...)
      {
         exit_status::set(127);
      }
   throw ysh_exit();
}

void fn_ls(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   dir files = state.files();
   if (words.size() == static_cast<size_t>(1))
   {
      cout << "/"
           << ((state.path()->size() > 0)
                   ? static_cast<string>(state.path()->back())
                   : "")
           << ":" << endl;
      for (auto iter : files)
         cout << setw(6) << iter.second->get_inode_nr()
              << "  " << setw(6) << iter.second->file()->size()
              << "  " << iter.first
              << (iter.second->type() ==
                              file_type::DIRECTORY_TYPE &&
                          iter.first != "." && iter.first != ".."
                      ? "/"
                      : "")
              << endl;
   }
   else
      for (string view :
           vector<string>(words.cbegin() + 1, words.cend()))
      {
         if (view == ".")
         {
            cout << ".:" << endl;
            for (auto iter : files)
               cout << setw(6) << iter.second->get_inode_nr()
                    << "  " << setw(6)
                    << iter.second->file()->size()
                    << "  " << iter.first
                    << (iter.second->type() ==
                                    file_type::DIRECTORY_TYPE &&
                                iter.first != "." &&
                                iter.first != ".."
                            ? "/"
                            : "")
                    << endl;
            continue;
         }
         wordvec oldpath = vector<string>(*state.path());
         fn_cd(state, vector<string>{"cd", view});
         cout << (view == "." ? "." : "/")
              << ((state.path()->size() > 0)
                      ? static_cast<string>(state.path()->back())
                      : "")
              << ":" << endl;
         for (auto iter : state.files())
            cout << setw(6) << iter.second->get_inode_nr()
                 << "  " << setw(6)
                 << iter.second->file()->size() << "  " << iter.first
                 << (iter.second->type() ==
                                 file_type::DIRECTORY_TYPE &&
                             iter.first != "." &&
                             iter.first != ".."
                         ? "/"
                         : "")
                 << endl;
         stringstream buffer;
         buffer << ".";
         for (string iter : oldpath)
            buffer << "/" << iter;
         fn_cd(state, vector<string>{"cd", buffer.str()});
      }
}

void fn_lsr(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   for (string view : vector<string>(words.cbegin(), words.cend()))
   {
      wordvec oldpath = vector<string>(*state.path());
      if (view == "lsr" &&
          words.size() > static_cast<size_t>(1))
         continue;
      fn_cd(state, vector<string>{"cd", (view ==
                                                 "lsr"
                                             ? "."
                                             : view)});
      cout << (view == "." ? "." : "/")
           << ((state.path()->size() > 0)
                   ? static_cast<string>(state.path()->back())
                   : "")
           << ":" << endl;
      static_cast<directory *>(state.cur()->file().get())
          ->lsr(state.cur(), "");
      stringstream buffer;
      buffer << ".";
      for (string iter : oldpath)
         buffer << "/" << iter;
      fn_cd(state, vector<string>{"cd", buffer.str()});
      fn_cd(state, vector<string>{"cd", ".."});
   }
}

void fn_make(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);

   if (words.size() == static_cast<size_t>(1))
      return;

   wordvec oldpath = vector<string>(*state.path());
   wordvec dirs = split(words[1], "/");
   string f = words[1];
   if (dirs.size() > 0)
   {
      f = dirs.back();
      dirs.pop_back();
      stringstream prebuff;
      prebuff << ".";
      for (string iter : dirs)
         prebuff << "/" << iter;
      fn_cd(state, vector<string>{"cd", prebuff.str()});
   }

   if (state.files().count(f) > 0 &&
       state.files()[f].get()->type() ==
           file_type::DIRECTORY_TYPE)
      throw file_error("make: " + f + ": Is a directory");
   inode_ptr newfile = state.files().count(f) > 0
                           ? state.files()[f]
                           : state.cur()->file().get()->mkfile(f);
   newfile->file()
       ->writefile(vector<string>(words.cbegin() + 2, words.cend()));

   if (dirs.size() > 0)
   {
      stringstream buffer;
      for (string iter : oldpath)
         buffer << iter << "/";
      fn_cd(state, vector<string>{"cd"});
      fn_cd(state, vector<string>{"cd", buffer.str()});
   }
}

void fn_mkdir(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() == static_cast<size_t>(1))
      return;

   wordvec oldpath = vector<string>(*state.path());
   wordvec dirs = split(words[1], "/");
   string p = words[1];
   if (dirs.size() > 0)
   {
      p = dirs.back();
      dirs.pop_back();
      stringstream prebuff;
      prebuff << ".";
      for (string iter : dirs)
         prebuff << "/" << iter;
      fn_cd(state, vector<string>{"cd", prebuff.str()});
   }

   string name = split(p, "/").back(); //path
   if (state.files().count(p) > 0)
      throw file_error("mkdir: " + p + ": Directory already exists");
   inode_ptr newdir = state.cur()->file().get()->mkdir(p);
   static_cast<directory *>(newdir.get()->file().get())
       ->init(state.cur(), newdir);

   if (dirs.size() > 0)
   {
      stringstream buffer;
      for (string iter : oldpath)
         buffer << iter << "/";
      fn_cd(state, vector<string>{"cd"});
      fn_cd(state, vector<string>{"cd", buffer.str()});
   }
}

void fn_prompt(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() == static_cast<size_t>(1))
      return;
   string newPrompt = "";
   for (int i = 1; i < words.size(); i++)
      newPrompt += words[i] + " ";
   state.setprompt(newPrompt);
}

void fn_pwd(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   // cout << state.path()->size() << endl;
   cout << "/"
        << ((state.path()->size() > 0)
                ? static_cast<string>(state.path()->back())
                : "")
        << endl;
}

void fn_rm(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() == static_cast<size_t>(1))
      return;
   for (string view : vector<string>(words.cbegin() + 1, words.cend()))
   {
      wordvec dirs = split(view, "/");
      string del = dirs.back();
      dirs.pop_back();
      stringstream prebuff;
      prebuff << ".";
      for (string iter : dirs)
         prebuff << "/" << iter;
      wordvec oldpath = vector<string>(*state.path());
      fn_cd(state, vector<string>{"cd", prebuff.str()});
      if (state.files().count(del) == 0)
         throw file_error("rm: " + del +
                          ": Is not a file or directory");
      if (
          state.files()[del].get()->type() ==
              file_type::DIRECTORY_TYPE &&
          state.files()[del].get()->file()->size() > 2)
         return;
      state.cur().get()->file()->remove(del);

      stringstream buffer;
      for (string iter : oldpath)
         buffer << iter << "/";
      fn_cd(state, vector<string>{"cd"});
      fn_cd(state, vector<string>{"cd", buffer.str()});
   }
}

void fn_rmr(inode_state &state, const wordvec &words)
{
   DEBUGF('c', state);
   DEBUGF('c', words);
   if (words.size() == static_cast<size_t>(1))
      return;
   for (string view : vector<string>(words.cbegin() + 1, words.cend()))
   {
      wordvec dirs = split(view, "/");
      string del = dirs.back();
      wordvec oldpath = vector<string>(*state.path());
      fn_cd(state, vector<string>{"cd", view});
      static_cast<directory *>(state.cur()->file().get())
          ->rmr(state.cur());
      fn_cd(state, vector<string>{"cd", ".."});
      static_cast<directory *>(state.cur()->file().get())
          ->remove(del);

      stringstream buffer;
      buffer << ".";
      for (string iter : oldpath)
         buffer << "/" << iter;
      fn_cd(state, vector<string>{"cd", buffer.str()});
   }
}
