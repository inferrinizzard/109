// $Id: shape.cpp,v 1.2 2019-02-28 15:24:20-08 - - $

#include <typeinfo>
#include <unordered_map>
#include <cstring>
#include <math.h>
using namespace std;

#include "shape.h"
#include "util.h"

static unordered_map<void *, string> fontname{
    {GLUT_BITMAP_8_BY_13, "Fixed-8x13"},
    {GLUT_BITMAP_9_BY_15, "Fixed-9x15"},
    {GLUT_BITMAP_HELVETICA_10, "Helvetica-10"},
    {GLUT_BITMAP_HELVETICA_12, "Helvetica-12"},
    {GLUT_BITMAP_HELVETICA_18, "Helvetica-18"},
    {GLUT_BITMAP_TIMES_ROMAN_10, "Times-Roman-10"},
    {GLUT_BITMAP_TIMES_ROMAN_24, "Times-Roman-24"},
};

ostream &operator<<(ostream &out, const vertex &where)
{
   out << "(" << where.xpos << "," << where.ypos << ")";
   return out;
}

shape::shape()
{
   DEBUGF('c', this);
}

text::text(void *glut_bitmap_font_, const string &textdata_)
    : glut_bitmap_font(glut_bitmap_font_), textdata(textdata_)
{
   DEBUGF('c', this);
}

ellipse::ellipse(GLfloat width, GLfloat height)
    : dimension({width, height})
{
   DEBUGF('c', this);
}

circle::circle(GLfloat diameter) : ellipse(diameter, diameter)
{
   DEBUGF('c', this);
}

polygon::polygon(const vertex_list &vertices_) : vertices(vertices_)
{
   DEBUGF('c', this);
}

rectangle::rectangle(GLfloat width, GLfloat height)
    : polygon(vertex_list{
          vertex{-width / 2, -height / 2},
          vertex{width / 2, -height / 2},
          vertex{width / 2, height / 2},
          vertex{-width / 2, height / 2}})
{
   DEBUGF('c', this << "(" << width << "," << height << ")");
}

square::square(GLfloat width) : rectangle(width, width)
{
   DEBUGF('c', this);
}

diamond::diamond(GLfloat width, GLfloat height)
    : polygon(vertex_list{
          vertex{-width / 2, 0},
          vertex{0, -height / 2},
          vertex{width / 2, 0},
          vertex{0, height / 2}}) {}

triangle::triangle(const vertex_list &verts) : polygon(verts) {}

equilateral::equilateral(const vertex_list &verts)
    : triangle(verts) {}

void text::draw(const vertex &center, const rgbcolor &color,
                bool selected, int width) const
{
   DEBUGF('d', this << "(" << center << "," << color
                    << "," << selected << "," << width << ")");
   auto outText = reinterpret_cast<const GLubyte *>(textdata.c_str());
   glColor3ubv(color.ubvec);
   glRasterPos2f(center.xpos, center.ypos);
   glutBitmapString(glut_bitmap_font, outText);
   glutSwapBuffers();
}

void ellipse::draw(const vertex &center, const rgbcolor &color,
                   bool selected, int width) const
{
   DEBUGF('d', this << "(" << center << "," << color << ")");
   glBegin(selected ? GL_LINE_LOOP : GL_POLYGON);
   if (selected)
      glLineWidth(width);
   glColor3ubv(color.ubvec);
   for (float theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / 32)
      glVertex2f(dimension.xpos * cos(theta) + center.xpos,
                 dimension.ypos * sin(theta) + center.ypos);
   glEnd();
}

void polygon::draw(const vertex &center, const rgbcolor &color,
                   bool selected, int width) const
{
   DEBUGF('d', this << "(" << center << "," << color << ")");
   glBegin(selected ? GL_LINE_LOOP : GL_POLYGON);
   if (selected)
      glLineWidth(width);
   glColor3ubv(color.ubvec);
   for (vertex v : vertices)
      glVertex2f(center.xpos + v.xpos, center.ypos + v.ypos);
   glEnd();
}

void shape::show(ostream &out) const
{
   out << this << "->" << demangle(*this) << ": ";
}

void text::show(ostream &out) const
{
   shape::show(out);
   out << glut_bitmap_font << "(" << fontname[glut_bitmap_font]
       << ") \"" << textdata << "\"";
}

void ellipse::show(ostream &out) const
{
   shape::show(out);
   out << "{" << dimension << "}";
}

void polygon::show(ostream &out) const
{
   shape::show(out);
   out << "{" << vertices << "}";
}

ostream &operator<<(ostream &out, const shape &obj)
{
   obj.show(out);
   return out;
}
