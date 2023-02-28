
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <vector>
#include <CImg.h>
struct Point;
class Triangle;

constexpr unsigned WIDTH = 1028;
constexpr unsigned HEIGHT = 1028;
//define the image to be a 2D array of Points(triple of floats)
typedef std::vector<std::vector<Point>> Image;//Each cell will hold RGB values
typedef std::vector<Triangle> Scene;//define the "Scene" type to be a vector of triangles


struct Point {
    float x, y, z;
    
};
//lets overload some operators so that it is comfortable to use the Point class
bool operator==(Point a, Point b)
{
    return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
}
bool operator!=(Point a, Point b)
{
    return !(a == b);
}
Point operator+(const Point& u, const Point& v)//adding two vectors
{
    return Point{ u.x + v.x, u.y + v.y, u.z + v.z };
}
Point operator-(const Point& u, const Point& v)//subtracting two vectors
{
    return Point{ u.x - v.x, u.y - v.y, u.z - v.z };
}
float operator*(const Point& u, const Point& v)//dot product
{
    return u.x * v.x + u.y * v.y + u.z * v.z;
}

Point operator*(float c, const Point& u)//scalar multiplication
{
    return Point{ u.x * c, u.y * c, u.z * c };
}

std::ostream& operator<<(std::ostream& os, const Point& p)
{
    os << "(" << p.x << ", " << p.y << ", " << p.z << ")";
    return os;
}

struct Ray {
    Point O;//the origin
    Point R;//the direction
    Point compute(float t)const { return O + t * R; }
};

std::ostream& operator<<(std::ostream& os, const Ray& r)
{
    
    os << "(" << r.O << "," << r.R << ")";

    return os;
}

inline Point cross(const Point& u, const Point& v)
{
    return Point{ u.y * v.z - u.z * v.y, v.x * u.z - u.x * v.z, u.x * v.y - u.y * v.x };
}

inline Point normalize(const Point& n)
{
    float norm = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    return Point{n.x/norm, n.y/norm, n.z/norm};
}


class PerspectiveCamera
{
private:
    float fov;
    Point pos;
    Point up;
    Point dir;
public:
    //by default fov is 90 degrees, camera is located at (0,0,0) up vector points up, and camera looks towards negative z axis
    PerspectiveCamera(float f = 1.570796f, Point p = { 0,0,0 }, Point d = { 0,0,-1 }, Point u = { 0,1,0 }) :fov{ f }, pos{ p }, up{ u }, dir{d}{}
    float getFov()const { return fov; }
    Point getPos()const { return pos; }
    Point getUp()const { return up; }
    Point getDir()const { return dir; }
};

Ray ConstructRayThroughPixel(const PerspectiveCamera& camera, unsigned i, unsigned j)
{
    float alfa = camera.getFov();
    Point dir = normalize(camera.getDir());
    //NDC space
    float Px = (i + 0.5f) / WIDTH;
    float Py = (j + 0.5f) / HEIGHT;
    //Screen space
    Px = 2 * Px - 1;
    Py = 1 - 2 * Py;
    //Account the aspect ratio
    Px = (Px * WIDTH) / HEIGHT;

    //Account fov
    Px = Px * tan(alfa / 2);
    Py = Py * tan(alfa / 2);

    //Create the point object
    Point res = Point{ Px, Py, 0 } + dir;
    return Ray{ camera.getPos(),res };

}

class Triangle {
    Point a;
    Point b;
    Point c;
    Point col;//The color value is also a triple of floats
public:
     Triangle(Point first, Point second, Point third, Point color) :a{ first }, b{ second }, c{ third }, col{color} {}
     Triangle(Point first, Point second, Point third) : Triangle{ first, second, third, {1.0f, 0.0f,0.0f} } {} //if no color is specified the triangle is red
     Point getA() const{ return a; }
     Point getB()const { return b; }
     Point getC() const { return c; }
     Point getCol() const  { return col; }
     void setA(const Point& A) { a = A; }
     void setB(const Point& B) { b = B; }
     void setC(const Point& C) { c = C; }
     void setCol(const Point& color) { col = color; }
     Point getNormal() const { return cross(a - c, b - c); }
     bool intersect(const Ray& ray, float& t) const
     {
         
         Point normal=getNormal();
         float D = normal * a;//the distance to the plane from the origin
         float epsilion = 0.0001f;
         if((normal * ray.R)>=0.0f&&(normal* ray.R)<epsilion)//if dot product is zero they don't intersect
         {
             return false;
         }


         t = (normal * ray.O + D) / (normal * ray.R);
         if (t < 0)return false;
         return true;
     }

     bool isInside(const Point& p)//inside outside test
     {
         Point normal = getNormal();
         Point e0 = a - c;
         Point e1 = b - a;
         Point e2 = c - b;
         Point c0 = p - c;
         Point c1 = p - a;
         Point c2 = p - b;
         if (normal * cross(e0, c0) < 0)return false;
         if (normal * cross(e1, c1) < 0)return false;
         if (normal * cross(e2, c2) < 0)return false;
         return true;
     }
};

Image RayCast(const PerspectiveCamera& camera, Scene triangles)
{
    Image data = std::vector<std::vector<Point>>(HEIGHT, std::vector<Point>(WIDTH, { 0,0,0 }));
    for (unsigned i = 0; i < HEIGHT; ++i)
    {
        for (unsigned j = 0; j < WIDTH; ++j)
        {
            float t_min = std::numeric_limits<float>::max();//t is +INFINITY at the beginning
            Point col{ 0,0,0 };
            Ray ray = ConstructRayThroughPixel(camera, i, j);
            for (Triangle k : triangles)
            {
                float t;
                if (k.intersect(ray, t)&&k.isInside(ray.compute(t)))
                {
                    if (t < t_min)
                    {
                        t_min = t;
                        col = k.getCol();
                   }
                }
            }
            data[i][j] = col;//The last color value stored defines the color of the pixel
        }
    }
    return data;
}
using namespace cimg_library;
int main()
{
   Triangle first{ { 0,1,-3}, {-1,-1, -3}, {1, -1, -3} , {1,1,0} };
   Triangle second{ { 0,2,-4}, {-2,-2, -4}, {2, -2, -4} , {0,1,0} };//the second triangle is slightly larger and behind the first
   Triangle third{ {-3, 1, -7}, {-3, -1, -7}, {3,-1, -7}, {1,0,0} };//the third one is partially behind
   Triangle forth{ { -0.5f,6.0f,-8.0f}, {-6.5f,-6.0f, -8.0f}, {5.0f, -6.0f, -8.0f} , {0.0f,0.0f,0.7f} };//largest triangle which is behind everyone
   Scene scene;
    scene.push_back(first);
    scene.push_back(second);
    scene.push_back(third);
    scene.push_back(forth);
    Image result = RayCast(PerspectiveCamera{ }, scene);//We are using default camera here, if you want you can change by giving appropriate parameters




    //save the result into bmp file
    CImg<unsigned char> img(HEIGHT, WIDTH, 1, 3);

    for (int i = 0; i < HEIGHT; ++i)
    {
        for (int j = 0; j < WIDTH; ++j)
        {
            Point temp = result[i][j];
            temp = 255 * temp;
           const  float col[] = { temp.x, temp.y , temp.z };
            img.draw_point(i, j, col);

        }
    }
    img.save("triangles.bmp");
   
}
