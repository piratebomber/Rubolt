/*
 * rubolt_math.h - Math Library Header for Rubolt
 * Example C header for wrapper generation
 */

#ifndef RUBOLT_MATH_H
#define RUBOLT_MATH_H

// Basic arithmetic
int rb_add(int a, int b);
int rb_subtract(int a, int b);
int rb_multiply(int a, int b);
float rb_divide(float a, float b);

// Advanced math
double rb_power(double base, double exp);
double rb_sqrt(double x);
double rb_abs(double x);

// Trigonometry
double rb_sin(double x);
double rb_cos(double x);
double rb_tan(double x);

// Utility
int rb_max(int a, int b);
int rb_min(int a, int b);
double rb_clamp(double value, double min, double max);

#endif // RUBOLT_MATH_H
