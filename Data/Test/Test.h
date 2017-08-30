
#if defined(K3D_CPP_REFLECTOR)
#define KCLASS(...) __attribute__((annotate(#__VA_ARGS__)))
#define KSTRUCT(...) __attribute__((annotate(#__VA_ARGS__)))
#define KFUNCTION(...) __attribute__((annotate(#__VA_ARGS__)))
#define KPROPERTY(...) __attribute__((annotate(#__VA_ARGS__)))
#define KENUM(...) __attribute__((annotate(#__VA_ARGS__)))
#else
#define KCLASS(...)  
#define KSTRUCT(...)  
#define KFUNCTION(...)  
#define KPROPERTY(...)  
#define KENUM(...)  
#endif


namespace Zen
{
    enum class Enum
    {
        E1,
        E2
    }KENUM();

    struct Struct
    {
        KPROPERTY()
        int X;
        KPROPERTY(Category=Data)
        int Y;

        KFUNCTION(BlueprintCallable)
        Struct() {}

    } KSTRUCT(Blueprint);

    class ClassWithoutVirtual
    {
    public:
        
    } KCLASS();
}