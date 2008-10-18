#include <iostream>

class Base {
public:
  virtual void method(int a) {
    std::cout << "Base::method(" << a << ");\n";
  }

  void method2(int a) {
    std::cout << "Base::method2(" << a << ");\n";
  }
};

class D1 : public Base {
public:
  void method(int a) {
    std::cout << "D1::method(" << a << ");\n";
  }
};

class D2 : public Base {
public:
  void method(int a) {
    std::cout << "D2::method(" << a << ");\n";
  }
};

int main() {
  Base *a, *b, *c;

  a = new D1();
  b = new D2();
  c = new Base();

  a->method(1);
  a->method2(2);

  b->method(3);
  b->method2(4);

  c->method(5);
  c->method2(6);

  delete a;
  delete b;
  delete c;

  return 0;
}
