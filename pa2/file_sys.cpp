// $Id: file_sys.cpp,v 1.6 2018-06-27 14:44:57-07 - - $

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <unordered_map>

using namespace std;

#include "debug.h"
#include "file_sys.h"

int inode::next_inode_nr{1};

struct file_type_hash
{
   size_t operator()(file_type type) const
   {
      return static_cast<size_t>(type);
   }
};

ostream &operator<<(ostream &out, file_type type)
{
   static unordered_map<file_type, string, file_type_hash> hash{
       {file_type::PLAIN_TYPE, "PLAIN_TYPE"},
       {file_type::DIRECTORY_TYPE, "DIRECTORY_TYPE"},
   };
   return out << hash[type];
}

inode_state::inode_state()
{
   DEBUGF('i', "root = "
                   << root << ", cwd = " << cwd
                   << ", prompt = \"" << prompt() << "\"");
   filepath = wordvec();
}

const string &inode_state::prompt() const
{
   return prompt_;
}

void inode_state::setprompt(const string p)
{
   prompt_ = p;
}

inode_ptr inode_state::cur()
{
   return cwd;
}

dir inode_state::files()
{
   return (*(static_cast<dir *>(cwd->file()->get())));
}

inode_ptr inode_state::top() { return root; }

wordvec *inode_state::path() { return &filepath; }

void inode_state::set(inode_ptr newdir)
{
   cwd = newdir;
   if (root == nullptr)
      root = newdir;
}

ostream &
operator<<(ostream &out, const inode_state &state)
{
   out << "inode_state: root = " << state.root
       << ", cwd = " << state.cwd;
   return out;
}

inode::inode(file_type type) : inode_nr(next_inode_nr++)
{
   ftype = type;
   switch (type)
   {
   case file_type::PLAIN_TYPE:
      contents = make_shared<plain_file>();
      break;
   case file_type::DIRECTORY_TYPE:
      contents = make_shared<directory>();
      break;
   }
   DEBUGF('i', "inode " << inode_nr << ", type = " << type);
}

int inode::get_inode_nr() const
{
   DEBUGF('i', "inode = " << inode_nr);
   return inode_nr;
}

base_file_ptr inode::file() { return contents; }

file_type inode::type() { return ftype; }

file_error::file_error(const string &what)
    : runtime_error(what) {}

size_t plain_file::size() const
{
   size_t size = data.size() > 0 ? data.size() - 1 : 0;
   for (string iter : data)
      for (auto i : iter)
         size++;
   DEBUGF('i', "size = " << size);
   return size;
}

const wordvec &plain_file::readfile() const
{
   DEBUGF('i', data);
   return data;
}

void plain_file::writefile(const wordvec &words)
{
   data = wordvec(words);
   DEBUGF('i', words);
}

void plain_file::remove(const string &)
{
   throw file_error("is a plain file");
}
inode_ptr plain_file::mkdir(const string &)
{
   throw file_error("is a plain file");
}
inode_ptr plain_file::mkfile(const string &)
{
   throw file_error("is a plain file");
}

void *plain_file::get() { return &data; }

directory::directory()
{
   dirents = dir();
   dirents.emplace(".", nullptr);
   dirents.emplace("..", nullptr);
}

void directory::init(inode_ptr parent, inode_ptr cur)
{
   dirents[".."] = parent;
   dirents["."] = cur;
}

size_t directory::size() const
{
   size_t size = dirents.size();
   DEBUGF('i', "size = " << size);
   return size;
}

const wordvec &directory::readfile() const
{
   throw file_error("is a directory");
}
void directory::writefile(const wordvec &)
{
   throw file_error("is a directory");
}

void directory::remove(const string &filename)
{
   if (dirents.count(filename) == 0)
      throw file_error(filename + " not found");
   inode_ptr del = dirents[filename];
   if (del->type() ==
           file_type::DIRECTORY_TYPE &&
       del->file()->size() > 2)
      throw file_error("cannot delete directory: " + filename);
   if (del->type() == file_type::DIRECTORY_TYPE)
      static_cast<dir *>(del->file()->get())->clear();
   else
      static_cast<wordvec *>(del->file()->get())->clear();
   dirents.erase(filename);
   DEBUGF('i', filename);
}

inode_ptr directory::mkdir(const string &dirname)
{
   if (dirents.count(dirname) > 0)
      throw file_error(dirname + " already exists");

   inode_ptr newdir = make_shared<inode>(file_type::DIRECTORY_TYPE);
   dirents.emplace(dirname, newdir);

   DEBUGF('i', dirname);
   return newdir;
}

inode_ptr directory::mkfile(const string &filename)
{
   inode_ptr newfile = make_shared<inode>(file_type::PLAIN_TYPE);
   dirents.emplace(filename, newfile);

   DEBUGF('i', filename);
   return newfile;
}

void *directory::get() { return &dirents; }

void directory::lsr(inode_ptr show, string relpath)
{
   dir files = (*static_cast<dir *>(show->file()->get()));
   for (auto iter : files)
      cout << setw(6)
           << iter.second->get_inode_nr()
           << "  " << setw(6) << iter.second->file()->size()
           << "  " << iter.first
           << (iter.second->type() == file_type::DIRECTORY_TYPE &&
                       iter.first != "." &&
                       iter.first != ".."
                   ? "/"
                   : "")
           << endl;
   for (auto iter : files)
   {
      if (iter.first == "." || iter.first == "..")
         continue;
      if (iter.second->type() == file_type::DIRECTORY_TYPE)
      {
         cout << relpath << "/" << iter.first << ":" << endl;
         //print full relative path
         lsr(iter.second, (relpath + "/" + iter.first));
      }
   }
}

void directory::rmr(inode_ptr del)
{
   wordvec deldir = vector<string>();
   for (auto iter =
            (*static_cast<dir *>(del->file()->get())).rbegin();
        iter != (*static_cast<dir *>(del->file()->get())).rend();
        ++iter)
      if (!(iter->first == "." || iter->first == ".."))
         deldir.push_back(iter->first);

   while (deldir.size() > 0)
   {
      if ((*static_cast<dir *>(del->file()
                                   ->get()))[deldir[0]]
                  ->type() ==
              file_type::DIRECTORY_TYPE &&
          (*static_cast<dir *>(del->file()->get()))[deldir[0]]
                  ->file()
                  ->size() > 0)
         rmr((*static_cast<dir *>(del->file()->get()))[deldir[0]]);
      remove(deldir[0].c_str());
      (*static_cast<dir *>(del->file()->get())).erase(deldir[0]);
   }
}
