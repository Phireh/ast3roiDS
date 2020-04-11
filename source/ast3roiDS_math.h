#ifndef AST3ROIDS_MATH_H
#define AST3ROIDS_MATH_H

// Generic functions
#define abs(a) _Generic((a), \
    int:   abs(a),  \
    float: absf(a), \
    double: absd(a))

/* Types */

typedef struct {
  float x;
  float y;
} vec2f;

typedef struct {
  float x;
  float y;
  float w;
  float h;
} rect;

typedef enum {
              X0,        // 0
              Y0,        // 1
              X1,        // 2
              Y1,        // 3
              X2,        // 4
              Y2,        // 5
              XY_TOTAL   // 6
} vert_idx_t;

/* Inline functions */

// Return the absolute value of a
float absf(float a)
{
  return a < 0 ? -a : a;
}

// Return the absolute value of a
double absd(double a)
{
  return a < 0 ? -a : a;
}

// Obtain random number up to a
float randf(float a)
{
  return (float)rand()/(float)(RAND_MAX/(a));
}

// Obtain random number in range [a,b]
float randf2(float a, float b)
{
  return abs(b) - randf(abs(b-a));
}

// Calculate the normal of 2D vector
inline float norm_2f(vec2f v)
{
  return sqrt((v.x*v.x) + (v.y*v.y));
}

// Normalize vector 2D in place
inline void normalize_2f(vec2f *v)
{
  float norm = norm_2f(*v);
  v->x /= norm;
  v->y /= norm;
}

// Takes an angle in degrees and computes the radian equivalent
inline float deg_to_rad(float degrees)
{
  return (degrees * M_PI) / 180.0f;
}

// Takes angle in rads and computes degree equivalent
inline float rad_to_deg(float rads)
{
  return (rads * 180.0f) / M_PI;
}

/* Special clamp to keep degrees between [0,360] but wrapping the values around */
inline float clamp_deg(float angle)
{
  return angle > 360.0f ? angle - 360.0f : angle < 0.0f ? angle + 360.0f : angle;
}

// Rotate a vector by angle in rads in place
inline void rotate_2f_rad(vec2f *v, float angle_in_rads)
{
  float x = v->x;
  float y = v->y;
  float rot_cos = cos(angle_in_rads);
  float rot_sin = sin(angle_in_rads);
  v->x = (rot_cos * x) - (rot_sin * y);
  v->y = (rot_sin * x) + (rot_cos * y);
}

// Rotate vector by angle in degrees in place
inline void rotate_2f_deg(vec2f *v, float angle_in_degs)
{
  float angle_in_rads = deg_to_rad(angle_in_degs);
  rotate_2f_rad(v, angle_in_rads);
}

// Scalar product of two 2D angles
inline float scalar_prod_2f(vec2f v1, vec2f v2)
{
  return (v1.x*v2.x) + (v1.y*v2.y);
}

// Get angle of 2D vector in respect to (1,0)
inline float vec_to_angle(vec2f v)
{
  normalize_2f(&v);
  float rads = atan2f(v.y, v.x);
  return rad_to_deg(rads);
}


// Check if point is inside rectangle
inline int inside_rect(float x, float y, float leftx, float rightx, float downy, float upy)
{
  return x > 0 && y > 0 && rightx - x > leftx && upy - y > downy;
}

// Check if point is inside circle
inline int inside_circle(float x, float y, float cx, float cy, float crad)
{
  return sqrt((cx - x) * (cx - x) + (cy - y) * (cy -y)) < crad;
}

// Limits a number inside range [floor, ceiling]
inline float clampf(float f, float floor, float ceiling)
{
  return f < floor ? floor : (f > ceiling ? ceiling : f);
}



/* TODO: Since we use STB's sprintf this is probably unneeded and might
 * be deleted soon */
/* Truncates float value into integral and fractional parts as integers.
   x : the float number
   i : destination of integral part
   f : destination of fractional part
   p : number of digits in fractional part
   return : -1 or +1 depending on the sign of x. */
inline int ftoi(float x, int *i, int *f, int p)
{
  float integral, fractional;
  fractional = modff(x, &integral);
  *i = (int) integral;
  *f = (int) abs((fractional * pow(10.0f,p)));
  return x >= 0.0f ? 1 : -1;
}

/* TODO: Since we use STB's sprintf this is probably unneeded and might
 * be deleted soon */
// Returns ' ' or '-' based on sign. Useful for visualization
inline char check_sign(int sign)
{
  return sign >= 0 ? ' ' : '-';
}


#endif
