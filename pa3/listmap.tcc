// $Id: listmap.tcc,v 1.12 2019-02-21 17:28:55-08 - - $

#include "listmap.h"
#include "debug.h"

//
/////////////////////////////////////////////////////////////////
// Operations on listmap::node.
/////////////////////////////////////////////////////////////////
//

//
// listmap::node::node (link*, link*, const value_type&)
//
template <typename Key, typename Value, class Less>
listmap<Key, Value, Less>::node::node(
    node *n, node *p,
    const value_type &v)
    : link(n, p), value(v) {}

//
/////////////////////////////////////////////////////////////////
// Operations on listmap.
/////////////////////////////////////////////////////////////////
//

//
// listmap::~listmap()
//
template <typename Key, typename Value, class Less>
listmap<Key, Value, Less>::~listmap()
{
   DEBUGF('l', reinterpret_cast<const void *>(this));
}

//
// iterator listmap::insert (const value_type&)
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::iterator
listmap<Key, Value, Less>::insert(const value_type &pair)
{
   DEBUGF('l', &pair << "->" << pair);
   node *newNode = new node(anchor(), anchor(), pair);
   if (this->empty())
   {
      anchor()->next = newNode;
      anchor()->prev = newNode;
      newNode->prev = anchor();
      newNode->next = anchor();
   }
   else
   {
      node *cur = anchor()->next;
      for (; cur->value.first < pair.first && cur->next != anchor(); cur = cur->next)
         ;
      if (cur->value.first != pair.first)
      {
         newNode->next = cur->next;
         newNode->prev = cur;
         cur->next->prev = newNode;
         cur->next = newNode;
      }
      else
         cur->value.second = pair.second;
   }
   return iterator(newNode);
}

//
// listmap::find(const key_type&)
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::iterator
listmap<Key, Value, Less>::find(const key_type &that)
{
   DEBUGF('l', that);
   iterator found(begin());
   for (; found != end() && less(found->first, that); ++found)
      ;
   return found;
}

//
// iterator listmap::erase (iterator position)
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::iterator
listmap<Key, Value, Less>::erase(iterator position)
{
   DEBUGF('l', &*position);
   iterator del(begin());
   for (; del != position; ++del)
      ;
   del.where->prev->next = del.where->next;
   del.where->next->prev = del.where->prev;
   node *next = del.where->next;
   delete del.where;
   return iterator(next);
}

//
/////////////////////////////////////////////////////////////////
// Operations on listmap::iterator.
/////////////////////////////////////////////////////////////////
//

//
// listmap::value_type& listmap::iterator::operator*()
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::value_type &
    listmap<Key, Value, Less>::iterator::operator*()
{
   DEBUGF('l', where);
   return where->value;
}

//
// listmap::value_type* listmap::iterator::operator->()
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::value_type *
    listmap<Key, Value, Less>::iterator::operator->()
{
   DEBUGF('l', where);
   return &(where->value);
}

//
// listmap::iterator& listmap::iterator::operator++()
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::iterator &
listmap<Key, Value, Less>::iterator::operator++()
{
   DEBUGF('l', where);
   where = where->next;
   return *this;
}

//
// listmap::iterator& listmap::iterator::operator--()
//
template <typename Key, typename Value, class Less>
typename listmap<Key, Value, Less>::iterator &
listmap<Key, Value, Less>::iterator::operator--()
{
   DEBUGF('l', where);
   where = where->prev;
   return *this;
}

//
// bool listmap::iterator::operator== (const iterator&)
//
template <typename Key, typename Value, class Less>
inline bool listmap<Key, Value, Less>::iterator::operator==(
    const iterator &that) const
{
   return this->where == that.where;
}

//
// bool listmap::iterator::operator!= (const iterator&)
//
template <typename Key, typename Value, class Less>
inline bool listmap<Key, Value, Less>::iterator::operator!=(
    const iterator &that) const
{
   return this->where != that.where;
}
