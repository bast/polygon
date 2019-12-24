use std::time::Instant;
extern crate rand;
use rand::Rng;

use polygons::io;
use polygons::structures::Edge;
use polygons::structures::Point;
use polygons::stuff;

fn main() {
    let mut polygons: Vec<Vec<Edge>> = Vec::new();

    let points: Vec<Point> = io::read_vector("tests/polygon.txt");
    let mut xs = Vec::new();
    let mut ys = Vec::new();
    for p in points.iter() {
        xs.push(p.x);
        ys.push(p.y);
    }

    let offset = 5.0;

    let num_points = xs.len();

    let num_blocks = 5;

    for i in 0..num_blocks {
        let polygon =
            stuff::create_polygon(num_points, &xs, i as f64 * offset, &ys, 0.0, i * num_points);
        polygons.push(polygon);
    }

    let mut nodes = Vec::new();
    for p in polygons.iter() {
        // group edges to nodes, 4 at the time
        nodes.append(&mut stuff::group_edges(4, p.clone()));
    }

    // we group nodes into a tree
    while nodes.len() > 1 {
        nodes = stuff::group_nodes(4, nodes);
    }

    let (x_min, x_max) = (-1.0, (num_blocks - 1) as f64 * offset + 2.0);
    let (y_min, y_max) = (-1.0, 2.0);

    let num_reference_points = 5_000_000;
    let mut rng = rand::thread_rng();
    let mut reference_points = Vec::new();

    for _ in 0..num_reference_points {
        reference_points.push(Point {
            x: rng.gen_range(x_min, x_max),
            y: rng.gen_range(y_min, y_max),
        });
    }

    let start = Instant::now();
    let _distances = stuff::get_distances_edge(&nodes, &reference_points);
    let (_indices, _distances) = stuff::get_distances_vertex(&nodes, &reference_points);
    let _contains = stuff::contains_points(&nodes, &reference_points);
    let duration = start.elapsed();

    println!("time elapsed in benchmark: {:?}", duration);
}
