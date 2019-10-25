#pragma once


class Geometry {
	Geometry();
	~Geometry();
};


struct Vector2f {
	Vector2f() :x(0), y(0) {}
	Vector2f(float inx, float iny) : x(inx), y(iny) {}
	float x, y;
	void operator=(const Vector2f& in) {
		x = in.x;
		y = in.y;
	}
	void operator+=(const Vector2f& in) {
		x += in.x;
		y += in.y;
	}
	void operator-=(const Vector2f& in) {
		x -= in.x;
		y -= in.y;
	}
	void operator*=(const float& in) {
		x *= in;
		y *= in;
	}
	void operator/=(const float& in) {
		x /= in;
		y /= in;
	}
	
	void Zero();

	float Magnitude() const;

	void Normalize();

	const Vector2f Normalized() const;

	// Vector3Å®VECTORÇ÷ïœä∑
};


Vector2f operator+(const Vector2f& lv, const Vector2f rv);
Vector2f operator-(const Vector2f& lv, const Vector2f rv);
Vector2f operator-(const Vector2f& lv, const float rv);
Vector2f operator-(const Vector2f& lv);
Vector2f operator*(const Vector2f& lv, const float rv);
Vector2f operator*(const float lv, const Vector2f rv);
Vector2f operator/(const Vector2f& lv, const float rv);
Vector2f operator/(const Vector2f& lv, const Vector2f& rv);


float Dot(const Vector2f& v1, const Vector2f& v2);
float Cross(const Vector2f& v1, const Vector2f& v2);


struct Vector2 {
	Vector2() :x(0), y(0) {}
	Vector2(int inx, int iny) : x(inx), y(iny) {}
	int x, y;
	void operator=(const Vector2& in) {
		x = in.x;
		y = in.y;
	}
	
	void operator+=(const Vector2& in) {
		x += in.x;
		y += in.y;
	}
	void operator-=(const Vector2& in) {
		x -= in.x;
		y -= in.y;
	}
	void operator*=(const int& in) {
		x *= in;
		y *= in;
	}
	void operator/=(const int& in) {
		x /= in;
		y /= in;
	}
};


Vector2 operator+(const Vector2& lv, const Vector2 rv);
Vector2 operator-(const Vector2& lv, const Vector2 rv);
Vector2 operator-(const Vector2& lv, const int rv);
Vector2 operator-(const Vector2& lv);
Vector2 operator*(const Vector2& lv, const int rv);
Vector2 operator*(const int lv, const Vector2 rv);
Vector2 operator/(const Vector2& lv, const int rv);
Vector2 operator/(const Vector2& lv, const Vector2& rv);


struct Size {
	Size();
	Size(int inx, int iny);
	int w;
	int h;
};

struct Rect {
	Vector2 center;
	Size size;
	Rect();
	Rect(int x, int y, int w, int h);
	Rect(const Vector2& pos, const Size& s);
	const int Left()const;
	const int Right()const;
	const int Top()const;
	const int Bottom()const;
	const int Width()const { return size.w; }
	const int Height()const { return size.h; }

	Size OverlapSize(Rect ra, Rect rb);

	///ãÈå`Çï`âÊÇ∑ÇÈ
	void Draw(unsigned int color = 0xffffffff);
	void Draw(const Vector2& offset, unsigned int color = 0xffffffff);
};

struct Circle {
	Vector2f pos;
	float radius;
	Circle();
	Circle(float x, float y, float r);
	Circle(const Vector2f& p, float r);
	void Draw(unsigned int color = 0xffffffff);
};

struct Segment {
	Vector2f start;
	Vector2f end;
	Segment();
	Segment(const Vector2f& p1, const Vector2f& p2);
	Segment(float ax, float ay, float bx, float by);
	void Draw(unsigned int color = 0xffffffff);
};
