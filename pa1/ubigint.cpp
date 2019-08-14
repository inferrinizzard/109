// $Id: ubigint.cpp,v 1.16 2019-04-02 16:28:42-07 - - $

#include <cctype>
#include <cstdlib>
#include <exception>
#include <stack>
#include <stdexcept>
using namespace std;

#include "ubigint.h"
#include "debug.h"

ubigint::ubigint(unsigned long that) : ubig_value(0)
{
   DEBUGF('~', this << " -> " << that)
   if (that == 0)
   {
      ubig_value.push_back('0');
      return;
   }
   for (long temp = that; temp != 0; temp /= 10)
      ubig_value.push_back('0' + (temp % 10));
}

ubigint::ubigint(const string &that) : ubig_value(0)
{
   DEBUGF('~', "that = \"" << that << "\"");
   for (char digit : that)
   {
      if (not isdigit(digit))
         throw invalid_argument("ubigint::ubigint(" + that + ")");
      ubig_value.insert(ubig_value.begin(), digit);
   }
}

ubigint ubigint::operator+(const ubigint &that) const
{
   ubigint result;
   int carry = 0;
   int size = ubig_value.size(), thatSize = that.ubig_value.size();
   for (int i = 0; i < max(thatSize, size); i++)
   {
      int digit;
      if (i >= thatSize && i < size)
         digit = (ubig_value[i] - '0') + carry;
      else if (i >= size && i < thatSize)
         digit = (that.ubig_value[i] - '0') + carry;
      else
         digit = (ubig_value[i] - '0') +
                 (that.ubig_value[i] - '0') + carry;
      carry = digit / 10;
      digit %= 10;
      result.ubig_value.push_back('0' + digit);
   }
   if (carry > 0)
      result.ubig_value.push_back('0' + carry);
   return result;
}

ubigint ubigint::operator-(const ubigint &that) const
{
   if (*this < that)
      throw domain_error("ubigint::operator-(a<b)");

   if (that == *this)
      return ubigint("0");

   ubigint result;
   int borrow = 0;
   int size = ubig_value.size(), thatSize = that.ubig_value.size();
   for (int i = 0; i < max(thatSize, size); i++)
   {
      int digit;
      if (i >= thatSize && i < size)
         digit = (ubig_value[i] - '0') - borrow;
      else if (i >= size && i < thatSize)
         digit = (that.ubig_value[i] - '0') - borrow;
      else
         digit = (ubig_value[i] - '0') -
                 (that.ubig_value[i] - '0') - borrow;

      borrow = 0;
      if (digit < 0 && i != max(thatSize, size) - 1)
      {
         digit += 10;
         borrow = 1;
      }

      result.ubig_value.push_back('0' + digit);
   }

   for (; result.ubig_value.size() > 0 &&
          (result.ubig_value.back() - '0') == 0;
        result.ubig_value.pop_back())
      ;
   return result;
}

ubigint ubigint::operator*(const ubigint &that) const
{
   if ((that.ubig_value.size() == 1 &&
        (that.ubig_value[0] - '0') == 0) ||
       (ubig_value.size() == 1 && (ubig_value[0] - '0') == 0))
      return ubigint("0");
   int size = ubig_value.size(), thatSize = that.ubig_value.size();

   ubigint product;

   if (thatSize == 1 && (that.ubig_value[0] - '0') == 2)
   {
      product = ubigint(*this);
      product.multiply_by_2();
      return product;
   }
   for (int i = 0; i < size + thatSize; i++)
      product.ubig_value.push_back('0' + 0);
   for (int i = 0; i < size; i++)
   {
      int c = 0;
      for (int j = 0; j < thatSize; j++)
      {
         int d = (product.ubig_value[i + j] - '0') +
                 (ubig_value[i] - '0') * (that.ubig_value[j] - '0') + c;
         product.ubig_value[i + j] = '0' + (d % 10);
         c = d / 10;
      }
      product.ubig_value[i + thatSize] = '0' + c;
   }
   for (; product.ubig_value.size() > 0 &&
          (product.ubig_value.back() - '0') == 0;
        product.ubig_value.pop_back())
      ;
   return product;
}

void ubigint::multiply_by_2()
{
   int carry = 0;
   int size = ubig_value.size();

   for (int i = 0; i < size; i++)
   {
      int digit = (ubig_value[i] - '0') * 2 + carry;
      carry = digit / 10;
      digit = digit % 10;

      ubig_value[i] = ('0' + digit);
   }
   if (carry > 0)
      ubig_value.push_back('0' + carry);
}

void ubigint::divide_by_2()
{
   int size = ubig_value.size();
   for (int i = 0; i < size; i++)
   {
      int num = (ubig_value[i] - '0') / 2;
      if (i < size - 1 && (ubig_value[i + 1] - '0') % 2 != 0)
      {
         num += 5;
         ubig_value[i + 1] = '0' + ((ubig_value[i + 1] - '0') - 1);
      }
      ubig_value[i] = '0' + num;
   }
   for (; ubig_value.size() > 0 && (ubig_value.back() - '0') == 0;
        ubig_value.pop_back())
      ;
}

struct quo_rem
{
   ubigint quotient;
   ubigint remainder;
};
quo_rem udivide(const ubigint &dividend, const ubigint &divisor_)
{
   // NOTE: udivide is a non-member function.
   ubigint divisor{divisor_};
   ubigint zero{0};
   if (divisor == zero)
      throw domain_error("udivide by zero");
   ubigint power_of_2{1};
   ubigint quotient{0};
   ubigint remainder{dividend}; // left operand, dividend

   while (divisor < remainder)
   {
      divisor.multiply_by_2();
      power_of_2.multiply_by_2();
   }
   while (power_of_2 > zero)
   {
      if (divisor <= remainder)
      {
         remainder = remainder - divisor;

         quotient = quotient + power_of_2;
      }
      divisor.divide_by_2();
      power_of_2.divide_by_2();
   }
   return {.quotient = quotient, .remainder = remainder};
}

ubigint ubigint::operator/(const ubigint &that) const
{
   if (that > *this)
      return ubigint("0");
   if (that == *this)
      return ubigint("1");

   int size = ubig_value.size(), thatSize = that.ubig_value.size();
   ubigint quotient;

   if (thatSize == 1 && (that.ubig_value[0] - '0') == 2)
   {
      if (size == 1 && (ubig_value[0] - '0') == 1)
         return ubigint("0");
      quotient = ubigint(*this);
      quotient.divide_by_2();
      return quotient;
   }
   return udivide(*this, that).quotient;
}

ubigint ubigint::operator%(const ubigint &that) const
{
   if (that > *this)
      return *this;
   if (that == *this)
      return ubigint("0");

   int thatSize = that.ubig_value.size();

   if (thatSize == 1 && (that.ubig_value[0] - '0') == 2)
      return ((ubig_value[0] - '0') % 2 == 0)
                 ? ubigint("0")
                 : ubigint("1");

   ubigint quotient;

   return udivide(*this, that).remainder;
}

bool ubigint::operator==(const ubigint &that) const
{
   int size = ubig_value.size(), thatSize = that.ubig_value.size();
   if (size != thatSize)
      return false;
   for (int i = 0; i < size; i++)
      if ((ubig_value[i] - '0') != (that.ubig_value[i] - '0'))
         return false;
   return true;
}

bool ubigint::operator<(const ubigint &that) const
{
   int size = ubig_value.size(), thatSize = that.ubig_value.size();
   if (size < thatSize)
      return true;
   if (thatSize < size)
      return false;

   for (int i = size - 1; i >= 0; i--)
   {
      if ((ubig_value[i] - '0') > (that.ubig_value[i] - '0'))
         return false;
      if ((ubig_value[i] - '0') < (that.ubig_value[i] - '0'))
         return true;
   }
   return false;
}

ostream &operator<<(ostream &out, const ubigint &that)
{
   string print = "";
   for (char digit : that.ubig_value)
      print = digit + print;
   return out << print;
}
