#include <limits>
#include <math.h>
#include <stdio.h>
#include <tuple>

#include "custom_functions.h"
#include "distance.h"
#include "edge.h"
#include "intersection.h"
#include "node.h"

// get best case distance to box
// rejected
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
double box_distance(const point p,
                    const double xmin,
                    const double xmax,
                    const double ymin,
                    const double ymax)
{
    double difx;

    if (p.x < xmin)
    {
        difx = p.x - xmin;
    }
    else if (p.x > xmax)
    {
        difx = p.x - xmax;
    }
    else
    {
        difx = 0.0;
    }

    double dify;

    if (p.y < ymin)
    {
        dify = p.y - ymin;
    }
    else if (p.y > ymax)
    {
        dify = p.y - ymax;
    }
    else
    {
        dify = 0.0;
    }

    return distance_squared(difx, dify);
}

bool skip_box_intersection(const point p,
                           const double xmax,
                           const double ymin,
                           const double ymax)
{
    if (p.x > xmax)
        return true;
    if (p.y > ymax)
        return true;
    if (p.y < ymin)
        return true;
    return false;
}

node::node()
{
    double large_number = std::numeric_limits<double>::max();

    // initialize bounding box for this node
    xmin = large_number;
    xmax = -large_number;
    ymin = large_number;
    ymax = -large_number;

    // initialize to a large number
    // we will minimize this number for each node
    // when building the tree
    h_min = large_number;
}

node::~node()
{
    children_nodes.clear();
    children_edges.clear();
}

double node::get_distance_edge(const double d, const point p) const
{
    if (box_distance(p, xmin, xmax, ymin, ymax) > d)
        return d;

    double d_ = d;

    if (children_nodes.size() > 0)
    {
        for (size_t i = 0; i < children_nodes.size(); i++)
        {
            d_ = std::min(d_, children_nodes[i].get_distance_edge(d_, p));
        }
        return d_;
    }

    if (children_edges.size() > 0)
    {
        for (size_t i = 0; i < children_edges.size(); i++)
        {
            d_ = std::min(d_,
                          dsegment(p.x,
                                   p.y,
                                   children_edges[i].p1.x,
                                   children_edges[i].p1.y,
                                   children_edges[i].p2.x,
                                   children_edges[i].p2.y));
        }
        return d_;
    }

    // this statement should never be reached
    // but is here to avoid compile warning
    return d_;
}

std::tuple<int, double>
node::get_distance_vertex(const int index, const double d, const point p) const
{
    if (box_distance(p, xmin, xmax, ymin, ymax) > d)
    {
        return std::make_tuple(index, d);
    }

    double d_ = d;
    int index_ = index;

    if (children_nodes.size() > 0)
    {
        for (size_t i = 0; i < children_nodes.size(); i++)
        {
            auto t = children_nodes[i].get_distance_vertex(index_, d_, p);
            double d_temp = std::get<1>(t);
            if (d_temp < d_)
            {
                d_ = d_temp;
                index_ = std::get<0>(t);
            }
        }
        return std::make_tuple(index_, d_);
    }

    if (children_edges.size() > 0)
    {
        for (size_t i = 0; i < children_edges.size(); i++)
        {
            double d_temp = distance_squared(children_edges[i].p1.x - p.x,
                                             children_edges[i].p1.y - p.y);
            if (d_temp < d_)
            {
                d_ = d_temp;
                index_ = children_edges[i].p1.index;
            }
        }
        double d_temp = distance_squared(
            children_edges[children_edges.size() - 1].p2.x - p.x,
            children_edges[children_edges.size() - 1].p2.y - p.y);
        if (d_temp < d_)
        {
            d_ = d_temp;
            index_ = children_edges[children_edges.size() - 1].p2.index;
        }
        return std::make_tuple(index_, d_);
    }

    // this statement should never be reached
    // but is here to avoid compile warning
    return std::make_tuple(index, d);
}

double node::get_distance_vertex_custom(const double d, const point p) const
{
    double f;
    f = g_function(sqrt(box_distance(p, xmin, xmax, ymin, ymax))) + h_min;

    // estimated minimum distance_function is larger than d
    // this means we can reject this node and return
    // if the estimate is not larger, we go down the tree
    if (f > d)
        return d;

    double d_ = d;

    if (children_nodes.size() > 0)
    {
        for (size_t i = 0; i < children_nodes.size(); i++)
        {
            d_ = std::min(d_,
                          children_nodes[i].get_distance_vertex_custom(d_, p));
        }
        return d_;
    }

    if (children_edges.size() > 0)
    {
        for (size_t i = 0; i < children_edges.size(); i++)
        {
            f = g_function(
                    sqrt(distance_squared(children_edges[i].p1.x - p.x,
                                          children_edges[i].p1.y - p.y))) +
                children_edges[i].p1.h;
            d_ = std::min(d_, f);
        }
        int last = children_edges.size() - 1;
        f = g_function(
                sqrt(distance_squared(children_edges[last].p2.x - p.x,
                                      children_edges[last].p2.y - p.y))) +
            children_edges[last].p2.h;
        d_ = std::min(d_, f);
        return d_;
    }

    // this statement should never be reached
    // but is here to avoid compile warning
    return d_;
}

int node::num_intersections(const int n, const point p) const
{
    if (skip_box_intersection(p, xmax, ymin, ymax))
        return n;

    int n_ = n;

    if (children_nodes.size() > 0)
    {
        for (size_t i = 0; i < children_nodes.size(); i++)
        {
            n_ = children_nodes[i].num_intersections(n_, p);
        }
        return n_;
    }

    if (children_edges.size() > 0)
    {
        for (size_t i = 0; i < children_edges.size(); i++)
        {
            if (crosses(p.x, p.y, children_edges[i]))
                n_++;
        }
        return n_;
    }

    // this statement should never be reached
    // but is here to avoid compile warning
    return n_;
}

void node::add_child_node(const node child)
{
    children_nodes.push_back(child);

    xmin = std::min(xmin, child.xmin);
    xmax = std::max(xmax, child.xmax);
    ymin = std::min(ymin, child.ymin);
    ymax = std::max(ymax, child.ymax);

    h_min = std::min(h_min, child.h_min);
}

void node::add_child_edge(const edge child)
{
    children_edges.push_back(child);

    xmin = std::min(xmin, child.p1.x);
    xmax = std::max(xmax, child.p1.x);
    ymin = std::min(ymin, child.p1.y);
    ymax = std::max(ymax, child.p1.y);

    xmin = std::min(xmin, child.p2.x);
    xmax = std::max(xmax, child.p2.x);
    ymin = std::min(ymin, child.p2.y);
    ymax = std::max(ymax, child.p2.y);

    h_min = std::min(h_min, child.p1.h);
    h_min = std::min(h_min, child.p2.h);
}
