#include <stdio.h>
#include <iostream>
#include <fstream>
#include <random>
#include <limits>

#include "polygon.h"
#include "intersection.h"
#include "distance.h"

#define AS_TYPE(Type, Obj) reinterpret_cast<Type *>(Obj)
#define AS_CTYPE(Type, Obj) reinterpret_cast<const Type *>(Obj)

void polygon_context::check_that_context_is_initialized() const
{
    if (not is_initialized)
    {
        fprintf(stderr, "ERROR: context is not initialized\n");
        exit(-1);
    }
}

POLYGON_API
polygon_context *polygon_new_context()
{
    return AS_TYPE(polygon_context, new polygon_context());
}
polygon_context::polygon_context()
{
    num_polygons = 0;
    is_initialized = true;
}

POLYGON_API
void polygon_free_context(polygon_context *context)
{
    if (!context)
        return;
    delete AS_TYPE(polygon_context, context);
}
polygon_context::~polygon_context()
{
    num_polygons = 0;

    bounding_box.clear();

    for (int i = 0; i < polygons_v.size(); i++)
    {
        polygons_v[i].clear();
    }
    polygons_v.clear();

    is_initialized = false;
}

POLYGON_API
void polygon_add_polygon(polygon_context *context,
                        const int num_points,
                        const double x[],
                        const double y[])
{
    AS_TYPE(polygon_context, context)->add_polygon(num_points, x, y);
}
void polygon_context::add_polygon(const int num_points,
                                 const double x[],
                                 const double y[])
{
    check_that_context_is_initialized();

    int ipolygon = num_polygons;
    num_polygons++;

    double large_number = std::numeric_limits<double>::max();

    bounding_box.push_back({point{large_number, large_number},
                            point{-large_number, -large_number}});

    std::vector<point> temp;

    for (int ipoint = 0; ipoint < num_points; ipoint++)
    {
        double x_ = x[ipoint];
        double y_ = y[ipoint];

        temp.push_back({x_, y_});

        bounding_box[ipolygon][0].x = std::min(bounding_box[ipolygon][0].x, x_);
        bounding_box[ipolygon][0].y = std::min(bounding_box[ipolygon][0].y, y_);
        bounding_box[ipolygon][1].x = std::max(bounding_box[ipolygon][1].x, x_);
        bounding_box[ipolygon][1].y = std::max(bounding_box[ipolygon][1].y, y_);
    }

    polygons_v.push_back(temp);
    temp.clear();
}

POLYGON_API
void polygon_contains_points(const polygon_context *context,
                            const int num_points,
                            const double x[],
                            const double y[],
                            bool contains_points[])
{
    AS_CTYPE(polygon_context, context)->contains_points(num_points, x, y, contains_points);
}
void polygon_context::contains_points(const int num_points,
                                     const double x[],
                                     const double y[],
                                     bool contains_points[]) const
{
    check_that_context_is_initialized();

    std::fill(&contains_points[0], &contains_points[num_points], false);
#pragma omp parallel for
    for (int ipoint = 0; ipoint < num_points; ipoint++)
    {
        for (int ipolygon = 0; ipolygon < num_polygons; ipolygon++)
        {
            if (!contains_points[ipoint])
            {
                // check whether we are not outside the bounding box
                if (x[ipoint] < bounding_box[ipolygon][0].x)
                    continue;
                if (y[ipoint] < bounding_box[ipolygon][0].y)
                    continue;
                if (x[ipoint] > bounding_box[ipolygon][1].x)
                    continue;
                if (y[ipoint] > bounding_box[ipolygon][1].y)
                    continue;

                int wn =
                    winding_number(x[ipoint], y[ipoint], polygons_v[ipolygon]);
                contains_points[ipoint] = (wn != 0);
            }
        }
    }
}


double distance_to_edge(const point p, const edge e)
{
    return dsegment(p.x, p.y, e.p1.x, e.p1.y, e.p2.x, e.p2.y);
}


node::node()
{
    double large_number = std::numeric_limits<double>::max();
    xmin = large_number;
    xmax = -large_number;
    ymin = large_number;
    ymax = -large_number;
}

node::~node()
{
}


// if best case distance is larger than currently optimum distance, this box is rejected
// the box is region 5:
//    |   |
//  1 | 2 | 3
// ___|___|___
//    |   |
//  4 | 5 | 6
// ___|___|___
//    |   |
//  7 | 8 | 9
//    |   |
bool skip_box(const double d,
              const point p,
              const double xmin,
              const double xmax,
              const double ymin,
              const double ymax)
{
    if (p.y > ymax)
    {
        // 1, 2, 3
        if (p.x < xmin)
        {
            // 1
            return distance_squared(p.x - xmin, p.y - ymax) > d;
        }
        else if (p.x > xmax)
        {
            // 3
            return distance_squared(p.x - xmax, p.y - ymax) > d;
        }
        else
        {
            // 2
            return distance_squared(0.0, p.y - ymax) > d;
        }
    }
    else if (p.y < ymin)
    {
        // 7, 8, 9
        if (p.x < xmin)
        {
            // 7
            return distance_squared(p.x - xmin, p.y - ymin) > d;
        }
        else if (p.x > xmax)
        {
            // 9
            return distance_squared(p.x - xmax, p.y - ymin) > d;
        }
        else
        {
            // 8
            return distance_squared(0.0, p.y - ymin) > d;
        }
    }
    else
    {
        // 4, 5, 6
        if (p.x < xmin)
        {
            // 4
            return distance_squared(p.x - xmin, 0.0) > d;
        }
        else if (p.x > xmax)
        {
            // 6
            return distance_squared(p.x - xmax, 0.0) > d;
        }
        else
        {
            // 5
            return false;
        }
    }
}
