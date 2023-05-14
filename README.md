# MSVC RTTI

Parses and symbolizes MSVC RTTI information in [Binary Ninja].

## Example Virtual Function Table Listing

Arguably the most import function of symbolizing RTTI information is the virtual function tables. The listing below is the symbolized view of `simple.cpp` (found in test\bins).

```c
void* data_140010320 = ParentA_objLocator  // ParentA
struct ParentA_vfTable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = ParentB::vFunc_0
    int64_t (* const vFunc_1)() = ParentA::vFunc_1
    int64_t (* const vFunc_2)() = ParentA::vFunc_2
}
void* data_140010340 = ParentB_objLocator  // ParentB
struct ParentB_vfTable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = ParentB::vFunc_0
    int64_t (* const vFunc_1)() = ParentB::vFunc_1
}
void* data_140010358 = SomeClass_objLocator  // SomeClass : ParentA, ParentB
struct SomeClass_vfTable = 
{
    void* (* const vFunc_0)(void* arg1, int32_t arg2) = SomeClass::vFunc_0
    int64_t (* const vFunc_1)() = ParentA::vFunc_1
    int64_t (* const vFunc_2)() = ParentA::vFunc_2
    int64_t (* const vFunc_3)() = SomeClass::vFunc_3
}
void* data_140010380 = SomeClass_objLocator  // __offset(8) SomeClass : ParentA, ParentB
struct SomeClass_vfTable = 
{
    int64_t (* const vFunc_0)(int64_t arg1, int32_t arg2) = SomeClass::vFunc_0
    int64_t (* const vFunc_1)() = ParentB::vFunc_1
}
void* data_140010398 = type_info_objLocator  // type_info
struct type_info_vfTable = 
{
    void*** (* const vFunc_0)(void*** arg1, char arg2) = type_info::vFunc_0
}
void* data_1400103a8 = std::exception_objLocator  // std::exception
struct std::exception_vfTable = 
{
    void*** (* const vFunc_0)(void*** arg1, char arg2) = std::exception::vFunc_0
    int64_t (* const vFunc_1)() = std::exception::vFunc_1
}
```

## Example Constructor Listing

Based off the information collected from the RTTI scan, we can deduce constructors and create types and symbolize their structures. Using the [type inheritence](https://binary.ninja/2023/05/03/3.4-finally-freed.html#inherited-types) in [Binary Ninja] we can make these types easily composable. The listing below shows the fully symbolized constructor function for `SomeClass` in `simple.cpp` (found in test\bins), as well as the accompanying auto created type.
```c
struct __base(ParentA, 0) __base(ParentB, 0) __data_var_refs SomeClass
{
    struct SomeClass_vfTable* vft_SomeClass;
    struct __ptr_offset(0x8) SomeClass_vfTable* vft_ptr_offset(0x8) SomeClass;
};

struct SomeClass* SomeClass::SomeClass(struct SomeClass* this)
{
    arg_8 = this;
    ParentA::ParentA(arg_8);
    ParentB::ParentB(&arg_8->vft_ptr_offset(0x8) SomeClass);
    arg_8->vft_SomeClass = &SomeClass_vfTable;
    arg_8->vft_ptr_offset(0x8) SomeClass = &__ptr_offset(0x8) SomeClass_vfTable;
    return arg_8;
}
```

## Example Virtual Function Listing

Using the newly created constructor object type in [Example Constructor Listing](#example-constructor-listing) we can apply it to all virtual functions as the first parameter. The listing below shows a fully symbolized virtual function for `SomeClass` in `simple.cpp` (found in test\bins).
```c
struct SomeClass* SomeClass::vFunc_0(struct SomeClass* this, int32_t arg2)
{
    sub_140001140(this);
    if ((arg2 & 1) != 0)
    {
        j_sub_140005f3c(this);
    }
    return this;
}
```

## TODO

- ~~Identify virtual functions~~ and integrate with component view.
- Provide a UI to view associated classes.
- Graphviz support.
- Automatic scan on binary open.
- Provide CI for releasing new versions automatically and on all platforms.
- Provide better logging.
- Fixup cross references between defined symbols and their RVA pointers.
- Provide statistics on discovered functions and other useful information after completion.

[Binary Ninja]: https://binary.ninja
