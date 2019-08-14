// $Id: interp.cpp,v 1.3 2019-03-19 16:18:22-07 - - $

#include <memory>
#include <string>
#include <vector>
#include <math.h>
using namespace std;

#include <GL/freeglut.h>

#include "debug.h"
#include "interp.h"
#include "shape.h"
#include "util.h"

static unordered_map<string, void *> fontcode{
    {"Fixed-8x13", GLUT_BITMAP_8_BY_13},
    {"Fixed-9x15", GLUT_BITMAP_9_BY_15},
    {"Helvetica-10", GLUT_BITMAP_HELVETICA_10},
    {"Helvetica-12", GLUT_BITMAP_HELVETICA_12},
    {"Helvetica-18", GLUT_BITMAP_HELVETICA_18},
    {"Times-Roman-10", GLUT_BITMAP_TIMES_ROMAN_10},
    {"Times-Roman-24", GLUT_BITMAP_TIMES_ROMAN_24},
};

unordered_map<string, interpreter::interpreterfn>
    interpreter::interp_map{
        {"define", &interpreter::do_define},
        {"draw", &interpreter::do_draw},
        {"border", &interpreter::do_border},
        {"moveby", &interpreter::do_move},
    };

unordered_map<string, interpreter::factoryfn>
    interpreter::factory_map{
        {"text", &interpreter::make_text},
        {"ellipse", &interpreter::make_ellipse},
        {"circle", &interpreter::make_circle},
        {"polygon", &interpreter::make_polygon},
        {"triangle", &interpreter::make_triangle},
        {"equilateral", &interpreter::make_equilateral},
        {"diamond", &interpreter::make_diamond},
        {"rectangle", &interpreter::make_rectangle},
        {"square", &interpreter::make_square},
    };

interpreter::shape_map interpreter::objmap;

interpreter::~interpreter()
{
   for (const auto &itor : objmap)
   {
      cout << "objmap[" << itor.first << "] = "
           << *itor.second << endl;
   }
}

void interpreter::interpret(const parameters &params)
{
   DEBUGF('i', params);
   if (params.size() < 4)
   {
      cout << "usage: <command> <args>" << endl;
      return;
   }
   param begin = params.cbegin();
   string command = *begin;
   auto itor = interp_map.find(command);
   if (itor == interp_map.end())
      throw runtime_error("syntax error");
   interpreterfn func = itor->second;
   func(++begin, params.cend());
}

void interpreter::do_define(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   string name = *begin;
   objmap.emplace(name, make_shape(++begin, end));
}

void interpreter::do_draw(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   if (end - begin != 4)
      throw runtime_error("syntax error");
   string name = begin[1];
   shape_map::const_iterator itor = objmap.find(name);
   if (itor == objmap.end())
   {
      throw runtime_error(name + ": no such shape");
   }
   rgbcolor color{begin[0]};
   vertex where{from_string<GLfloat>(begin[2]),
                from_string<GLfloat>(begin[3])};
   window::push_back(object(itor->second, where, color));
}

void interpreter::do_border(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   rgbcolor colour = rgbcolor(*begin++);
   int weight = stoi(*begin);
   window::setborder(colour);
   window::setstroke(weight);
}

void interpreter::do_move(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   window::setdelta(stof(*begin));
}

shape_ptr interpreter::make_shape(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   string type = *begin++;
   auto itor = factory_map.find(type);
   if (itor == factory_map.end())
   {
      throw runtime_error(type + ": no such shape");
   }
   factoryfn func = itor->second;
   return func(begin, end);
}

shape_ptr interpreter::make_text(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   void *infont = fontcode[*begin++];
   string inText = "";
   for (int i = 0; begin != end; i++)
      inText.append((i == 0 ? "" : " ") + *begin++);
   return make_shared<text>(infont, inText);
}

shape_ptr interpreter::make_ellipse(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   float width = stof(*begin++), height = stof(*begin);
   return make_shared<ellipse>(width, height);
}

shape_ptr interpreter::make_circle(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   return make_shared<circle>(stof(*begin));
}

shape_ptr interpreter::make_polygon(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   vertex_list points{};
   while (begin != end)
      points.push_back(vertex{stof(*begin++), stof(*begin++)});
   float xbar = 0, ybar = 0;
   for (vertex v : points)
   {
      xbar += v.xpos;
      ybar += v.ypos;
   }
   vertex core{xbar / points.size(), ybar / points.size()};
   vertex_list ptsNorm{};
   for (vertex v : points)
      ptsNorm.push_back(vertex{v.xpos - core.xpos, v.ypos - core.ypos});
   return make_shared<polygon>(ptsNorm);
}

shape_ptr interpreter::make_triangle(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   vertex_list points{};
   while (begin != end)
      points.push_back(vertex{stof(*begin++), stof(*begin++)});
   float xbar = 0, ybar = 0;
   for (vertex v : points)
   {
      xbar += v.xpos;
      ybar += v.ypos;
   }
   vertex core{xbar / 3, ybar / 3};
   vertex_list ptsNorm{};
   for (vertex v : points)
      ptsNorm.push_back(vertex{v.xpos - core.xpos, v.ypos - core.ypos});
   return make_shared<triangle>(ptsNorm);
}

shape_ptr interpreter::make_rectangle(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   float width = stof(*begin++), height = stof(*begin);
   return make_shared<rectangle>(width, height);
}

shape_ptr interpreter::make_square(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   return make_shared<square>(stof(*begin));
}

shape_ptr interpreter::make_diamond(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   float width = stof(*begin++), height = stof(*begin);
   return make_shared<diamond>(width, height);
}

shape_ptr interpreter::make_equilateral(param begin, param end)
{
   DEBUGF('f', range(begin, end));
   float side = stof(*begin);
   return make_shared<equilateral>(vertex_list{
       vertex{-side / 2, -side / 2 / sqrtf(3)},
       vertex{side / 2, -side / 2 / sqrtf(3)},
       vertex{0, side / sqrtf(3)}});
}
