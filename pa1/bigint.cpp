// $Id: bigint.cpp,v 1.78 2019-04-03 16:44:33-07 - - $

#include <cstdlib>
#include <exception>
#include <stack>
#include <stdexcept>
#include <sstream>
using namespace std;

#include "bigint.h"
#include "debug.h"
#include "relops.h"

bigint::bigint(long that) : uvalue(that), is_negative(that < 0)
{
   DEBUGF('~', this << " -> " << uvalue);
}

bigint::bigint(const ubigint &uvalue_, bool is_negative_)
    : uvalue(uvalue_), is_negative(is_negative_)
{
   is_negative = is_negative_;
   uvalue = uvalue_;
}

bigint::bigint(const string &that)
{
   is_negative = that.size() > 0 and that[0] == '_';
   uvalue = ubigint(that.substr(is_negative ? 1 : 0));
}

bigint bigint::operator+() const
{
   return *this;
}

bigint bigint::operator-() const
{
   return {uvalue, not is_negative};
}

bigint bigint::operator+(const bigint &that) const
{
   bigint result;
   if (is_negative == that.is_negative)
   {
      result.uvalue = uvalue + that.uvalue;
      result.is_negative = is_negative;
   }
   else
   {
      if (uvalue == that.uvalue)
         return {ubigint("0"), false};
      result.uvalue = max(uvalue, that.uvalue) -
                      min(uvalue, that.uvalue);
      result.is_negative = uvalue > that.uvalue
                               ? is_negative
                               : that.is_negative;
   }
   return result;
}

bigint bigint::operator-(const bigint &that) const
{
   bigint result;
   if (is_negative != that.is_negative)
   {
      result.uvalue = uvalue + that.uvalue;
      result.is_negative = is_negative;
   }
   else
   {
      if (uvalue == that.uvalue)
         return {ubigint("0"), false};
      result.uvalue = max(uvalue, that.uvalue) -
                      min(uvalue, that.uvalue);
      result.is_negative = that.uvalue > uvalue
                               ? !that.is_negative
                               : is_negative;
   }
   return result;
}

bigint bigint::operator*(const bigint &that) const
{
   return {uvalue * that.uvalue, is_negative || that.is_negative};
}

bigint bigint::operator/(const bigint &that) const
{
   return {uvalue / that.uvalue, is_negative || that.is_negative};
}

bigint bigint::operator%(const bigint &that) const
{
   return {uvalue % that.uvalue, is_negative || that.is_negative};
}

bool bigint::operator==(const bigint &that) const
{
   return is_negative == that.is_negative and uvalue == that.uvalue;
}

bool bigint::operator<(const bigint &that) const
{
   if (is_negative != that.is_negative)
      return is_negative;
   return is_negative ? uvalue > that.uvalue
                      : uvalue < that.uvalue;
}

ostream &operator<<(ostream &out, const bigint &that)
{
   stringstream buffer;
   buffer << that.uvalue;
   string num = buffer.str();
   if (that.is_negative)
      out << "-";
   int rows = 0;
   string acc = "";
   int size = num.length();
   for (int i = 0; i < size; i++)
   {
      if ((that.is_negative && i == 68) ||
          (!that.is_negative && i == 69) ||
          (i > 69 && i % 69 == 0))
      // if (i > 0 &&
      //     i % (68 - (that.is_negative && rows == 0 ? 1 : 0)) == 0 &&
      //     acc.length() > 1)
      {
         out << acc << '\\' << '\n';
         acc = "";
         rows++;
      }
      acc += num[i];
   }
   return out << acc;
}
