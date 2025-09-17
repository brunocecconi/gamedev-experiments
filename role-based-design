#pragma once

#include <tuple>
#include <type_traits>
#include <iostream>
#include <string>
#include <utility> // For std::forward

// --- Tier 0: Core Metaprogramming Utilities ---

// A simple utility to hold a list of types.
template <typename... T>
struct TypeList {};

// A minimal, zero-cost tuple implementation for holding components.
// It is fully encapsulated and uses [[no_unique_address]] for optimization.
template <typename... Types>
class SimpleTuple;

template <>
class SimpleTuple<> {};

template <typename HeadType, typename... TailTypes>
class SimpleTuple<HeadType, TailTypes...> {
private:
    [[no_unique_address]] HeadType Head;
    [[no_unique_address]] SimpleTuple<TailTypes...> Tail;

    template <typename T>
    struct IndexOf;

    // Correctly uses a TypeList to unpack for recursion
    template <typename T, typename First, typename... Rest>
    struct IndexOf<T, TypeList<First, Rest...>> {
        static constexpr size_t value = std::is_same_v<T, First> ? 0 : 1 + IndexOf<T, TypeList<Rest...>>::value;
    };

    template <size_t N>
    constexpr auto& GetByIndexImpl() {
        if constexpr (N == 0) {
            return Head;
        } else {
            return Tail.template GetByIndexImpl<N - 1>();
        }
    }

    template <size_t N>
    constexpr const auto& GetByIndexImpl() const {
        if constexpr (N == 0) {
            return Head;
        } else {
            return Tail.template GetByIndexImpl<N - 1>();
        }
    }

public:
    SimpleTuple() = default;

    template<typename HeadArg, typename... TailArgs>
    constexpr SimpleTuple(HeadArg&& head, TailArgs&&... tail)
        : Head(std::forward<HeadArg>(head)), Tail(std::forward<TailArgs>(tail)...)
    {}

    template <typename T>
    constexpr T& Get() {
        constexpr size_t Index = IndexOf<T, TypeList<HeadType, TailTypes...>>::value;
        return this->template GetByIndexImpl<Index>();
    }

    template <typename T>
    constexpr const T& Get() const {
        constexpr size_t Index = IndexOf<T, TypeList<HeadType, TailTypes...>>::value;
        return this->template GetByIndexImpl<Index>();
    }
};


// --- Tier 1: Marker Structs ---

// Marker structs for compile-time categorization (zero-cost).
struct Role {};
struct Attribute {};


// --- Tier 2: The 'Composition' CRTP Base Class (Hardened Version) ---

template <typename Derived, typename RolesList, typename AttributesList>
class Composition;

template <typename Derived, typename... TRoles, typename... TAttributes>
class Composition<Derived, TypeList<TRoles...>, TypeList<TAttributes...>> {
private:
    SimpleTuple<TRoles...>    RolesTuple;
    SimpleTuple<TAttributes...> AttributesTuple;

    // --- Compile-Time Dependency and Rule Enforcement ---
    template<typename T, typename... TPack>
    static constexpr bool IsTypeInPack = (std::is_same_v<T, TPack> || ...);

    template<typename T> struct CheckRoleDependencies;
    template<typename... TRequired>
    struct CheckRoleDependencies<TypeList<TRequired...>>
    {
        static constexpr bool AllPresent = (IsTypeInPack<TRequired, TAttributes...> && ...);
    };

    static constexpr bool DependenciesMet = (CheckRoleDependencies<typename TRoles::RequiredAttributes>::AllPresent && ...);
    static_assert(DependenciesMet, "Composition Error: An object is missing a required attribute for one of its roles.");

    static_assert((std::is_aggregate_v<TAttributes> && ...),
        "Composition Error: An Attribute type is not an aggregate. Attributes must be simple data structs.");

public:
    // This constructor enforces that all components are initialized.
    template<typename... RoleArgs, typename... AttributeArgs>
    constexpr Composition(TypeList<RoleArgs...>, TypeList<AttributeArgs...>)
        : RolesTuple(std::forward<RoleArgs>(RoleArgs)...),
          AttributesTuple(std::forward<AttributeArgs>(AttributeArgs)...)
    {
        static_assert(sizeof...(TRoles) == sizeof...(RoleArgs),
            "Composition Error: Incorrect number of Roles provided to the constructor.");
        static_assert(sizeof...(TAttributes) == sizeof...(AttributeArgs),
            "Composition Error: Incorrect number of Attributes provided to the constructor.");
    }

    // --- Public API ---
    template<typename T> static constexpr bool HasRole() { return IsTypeInPack<T, TRoles...>; }
    template<typename T> static constexpr bool HasAttribute() { return IsTypeInPack<T, TAttributes...>; }

    template<typename T>
    constexpr T& Role() {
        static_assert(HasRole<T>(), "Attempted to access a Role that does not exist on this Composition.");
        return RolesTuple.template Get<T>();
    }

    template<typename T>
    constexpr const T& Role() const {
        static_assert(HasRole<T>(), "Attempted to access a Role that does not exist on this Composition.");
        return RolesTuple.template Get<T>();
    }

    template<typename T>
    constexpr T& Attribute() {
        static_assert(HasAttribute<T>(), "Attempted to access an Attribute that does not exist on this Composition.");
        return AttributesTuple.template Get<T>();
    }

    template<typename T>
    constexpr const T& Attribute() const {
        static_assert(HasAttribute<T>(), "Attempted to access an Attribute that does not exist on this Composition.");
        return AttributesTuple.template Get<T>();
    }
};


// --- EXAMPLE USAGE ---
// To compile and run this example, define the following macro:
// #define COMPOSITION_ENABLE_EXAMPLES

#ifdef COMPOSITION_ENABLE_EXAMPLES

// --- 1. Define Attributes ---
// Attributes are simple, aggregate data structs.
struct Transform : public Attribute {
    float X = 0.0f, Y = 0.0f, Z = 0.0f;
};

struct Category : public Attribute {
    std::string Name = "Default";
    // Methods are fine on aggregates as long as there's no user-provided constructor.
    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }
};


// --- 2. Define Roles ---
// Roles provide behavior and can have constructors and private state.
class Logger : public Role {
public:
    // This role has no hard dependencies on attributes.
    using RequiredAttributes = TypeList<>;

    explicit Logger(const std::string& DefaultContext) : Context(DefaultContext) {}

    template<typename HostType>
    void Log(const HostType& InHost, const std::string& Message) const {
        std::string CategoryName = Context;
        // It opportunistically checks for an attribute to add more context.
        if constexpr (HostType::HasAttribute<Category>()) {
            CategoryName = InHost.Attribute<Category>().GetName();
        }
        std::cout << "[" << CategoryName << "] " << Message << std::endl;
    }
private:
    std::string Context;
};

class Mover : public Role {
public:
    // This role DECLARES A HARD REQUIREMENT on the Transform attribute.
    using RequiredAttributes = TypeList<Transform>;

    template<typename HostType>
    void MoveX(HostType& InHost, float DeltaX) {
        // No need for `if constexpr` here; the compiler guarantees this attribute exists.
        InHost.Attribute<Transform>().X += DeltaX;
    }
};


// --- 3. Define a Concrete Object using Composition ---
class Player : public Composition<Player,
    TypeList<Logger, Mover>,
    TypeList<Transform, Category>
>
{
public:
    Player(const std::string& InName)
        // The constructor is responsible for constructing all base components.
        : Composition(
            TypeList<Logger, Mover>( Logger(InName), Mover() ),
            TypeList<Transform, Category>( Transform{100.0f, 0, 0}, Category{} )
        )
    {
        // We can further configure attributes after initial construction.
        Attribute<Category>().SetName(InName);
    }

    void Update() {
        Role<Mover>().MoveX(*this, 5.0f);
        Role<Logger>().Log(*this, "Update Finished.");
    }
};


// --- 4. Main function to run the example ---
int main() {
    Player MyPlayer("Test");
    std::cout << "Initial Position: " << MyPlayer.Attribute<Transform>().X << std::endl;
    MyPlayer.Update();
    std::cout << "Final Position: " << MyPlayer.Attribute<Transform>().X << std::endl;

    // This demonstrates the compile-time safety. Uncommenting the following
    // class definition would cause a static_assert to fire with a clear error message.
    /*
    class InvalidObject : public Composition<InvalidObject,
        TypeList<Mover>, // Has Mover role...
        TypeList<Category> // ...but is missing the required Transform attribute.
    > {};
    */

    return 0;
}

#endif // COMPOSITION_ENABLE_EXAMPLES
