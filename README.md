# MSVC RTTI

Parses and symbolizes MSVC RTTI information in [Binary Ninja].

## Example Virtual Function Table Listing

Arguably the most import function of symbolizing RTTI information is the virtual function tables. The listing below is the symbolized view of `simple.cpp` (found in test\bins).

```c
void* data_140010320 = ParentA::`RTTI Complete Object Locator
struct ParentA::VTable ParentA::`vftable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = ParentB::vFunc_0
    int64_t (* const vFunc_1)() __pure = ParentA::vFunc_1
    int64_t (* const vFunc_2)() __pure = ParentA::vFunc_2
}
void* data_140010340 = ParentB::`RTTI Complete Object Locator
struct ParentB::VTable ParentB::`vftable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = ParentB::vFunc_0
    int64_t (* const vFunc_1)() __pure = ParentB::vFunc_1
}
void* data_140010358 = SomeClass::`RTTI Complete Object Locator
struct SomeClass::VTable SomeClass::`vftable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = SomeClass::vFunc_0
    int64_t (* const vFunc_1)() __pure = ParentA::vFunc_1
    int64_t (* const vFunc_2)() __pure = ParentA::vFunc_2
    int64_t (* const vFunc_3)() __pure = SomeClass::vFunc_3
}
void* data_140010380 = SomeClass::`RTTI Complete Object Locator{for `ParentB}
struct ParentB::VTable SomeClass::`vftable{for `ParentB} = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = SomeClass::vFunc_0
    int64_t (* const vFunc_1)() __pure = ParentB::vFunc_1
}
```

## Example Constructor Listing

Based off the information collected from the RTTI scan, we can deduce constructors and create types and symbolize their structures. Using the [type inheritence](https://binary.ninja/2023/05/03/3.4-finally-freed.html#inherited-types) in [Binary Ninja] we can make these types easily composable. The listing below shows the fully symbolized constructor function for `Bird` in `overrides.cpp` (found in test\bins), as well as the accompanying auto created type.

```cpp
class __base(Animal, 0) __base(Flying, 0) Bird
{
    struct `Bird::VTable`* vtable;
    char const* field_8;
    struct `Flying::VTable`* vtable_Flying;
    int32_t field_18;
    __padding char _1C[4];
    int32_t field_20;
};

class Bird* Bird::Bird(class Bird* this, int32_t arg2)
{
    Animal::Animal(this);
    Flying::Flying(&this->vtable_Flying);
    this->vtable = &Bird::`vftable';
    this->vtable_Flying = &Bird::`vftable'{for `Flying};
    this->field_8 = "A bird";
    this->field_18 = 0x58;
    this->field_20 = arg2;
    return this;
}
```

## Example Virtual Function Listing

Using the newly created constructor object type in [Example Constructor Listing](#example-constructor-listing) we can apply it to all virtual functions as the first parameter. The listing below shows a fully symbolized virtual function for `Bird` in `overrides.cpp` (found in test\bins).

```c
uint64_t Bird::vFunc_0(class Bird* this)
{
    int32_t var_18 = 0;
    uint64_t field_20;
    while (true)
    {
        field_20 = ((uint64_t)this->field_20);
        if (var_18 >= field_20)
        {
            break;
        }
        fputs("Tweet!");
        var_18 = (var_18 + 1);
    }
    return field_20;
}

```

[Binary Ninja]: https://binary.ninja
