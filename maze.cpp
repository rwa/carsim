
#include "maze.hpp"

int Segment::id_count = 0;

std::optional<Point> getIntersection(const Ray& ray, const LineSeg& segment) {
    // LineSeg direction vector
    Point segDir = {segment.end.x - segment.start.x, segment.end.y - segment.start.y};

    printf("segDir = (%f,%f)\n",segDir.x,segDir.y);

    // Calculate the determinant
    double det = -ray.direction.x * segDir.y + ray.direction.y * segDir.x;

    printf("det = %f\n",det);
    
    if (std::abs(det) < 1e-8) {
        // The lines are parallel (or coincident, if they lie on top of each other)
        return {};
    }

    // Calculate the parametric distance along each line where the intersection occurs
    double u = -(-ray.direction.y * (ray.origin.x - segment.start.x) + ray.direction.x * (ray.origin.y - segment.start.y)) / det;
    double t = -(segDir.x * (ray.origin.y - segment.start.y) - segDir.y * (ray.origin.x - segment.start.x)) / det;

    printf("u = %f\n",u);
    printf("t = %f\n",t);

    printf("ray origin x = %f\n",ray.origin.x);
    printf("ray direction x = %f\n",ray.direction.x);

    if (t >= 0 && u >= 0 && u <= 1) {
        // The intersection point is on the segment and in the ray's direction
        return Point{ray.origin.x + t * ray.direction.x, ray.origin.y + t * ray.direction.y};
    }

    // The intersection point is not in the ray's direction or not on the segment
    return {};
}

// Function to compute the intersection of a ray and a segment.
std::optional<Point> RayLineSegIntersect(const Ray& ray, const LineSeg& segment) {
  // Define vectors.
  Point r = ray.direction;
  Point s = {segment.end.x - segment.start.x, segment.end.y - segment.start.y};
  Point diff = {ray.origin.x - segment.start.x, ray.origin.y - segment.start.y};

  // Compute cross products.
  double rxs = r.x * s.y - r.y * s.x;
  double qpxr = diff.x * r.y - diff.y * r.x;
  double qpxs = diff.x * s.y - diff.y * s.x;

  // If r x s = 0 and (q - p) x r = 0, then the two lines are collinear.
  if (std::abs(rxs) < 1e-10 && std::abs(qpxr) < 1e-10) {
    // If the two lines are collinear, then the intersection of the ray and segment is not well-defined.
    return std::nullopt;
  }

  // If r x s = 0 and (q - p) x r != 0, then the two lines are parallel.
  if (std::abs(rxs) < 1e-10 && !(std::abs(qpxr) < 1e-10)) {
    return std::nullopt;
  }

  // t = (q - p) x s / (r x s)
  double t = qpxs / rxs;

  // u = (q - p) x r / (r x s)
  double u = qpxr / rxs;
  
  // If r x s != 0 and 0 <= u <= 1 and t >= 0, then the ray and segment intersect.
  if (!(std::abs(rxs) < 1e-10) && (0 <= u && u <= 1 && t >= 0)) {
    // Intersection point.
    Point intersection = {ray.origin.x + t * r.x, ray.origin.y + t * r.y};
    return intersection;
  }

  // Otherwise, the ray and segment do not intersect.
  return std::nullopt;
}
