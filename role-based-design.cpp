
#include <tuple>
#include <type_traits>
#include <iostream>
#include <string>
#include <utility> // For std::forward

// --- Tier 0: Core Metaprogramming Utilities ---
template <typename... T>
struct TypeList {};

template <typename... Types>
class SimpleTuple; // Assume our robust SimpleTuple exists

// --- Tier 1: Marker Structs (Hardened) ---
struct Attribute {};

// The Role marker now provides a default for RequiredAttributes.
struct Role {
    using RequiredAttributes = TypeList<>;
};


// --- Tier 2: The 'Composition' CRTP Base Class (Version 2.0) ---
template <typename Derived, typename RolesList, typename AttributesList>
class Composition;

template <typename Derived, typename... TRoles, typename... TAttributes>
class Composition<Derived, TypeList<TRoles...>, TypeList<TAttributes...>> {
private:
    std::tuple<TRoles...>    RolesTuple; // Using std::tuple for robustness
    std::tuple<TAttributes...> AttributesTuple;

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
    // --- FIXED: The constructor now accepts component arguments directly. ---
    template<typename... RoleArgs, typename... AttributeArgs>
    constexpr Composition(RoleArgs&&... roleArgs, AttributeArgs&&... attributeArgs)
        : RolesTuple(std::forward<RoleArgs>(roleArgs)...),
          AttributesTuple(std::forward<AttributeArgs>(attributeArgs)...)
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
        return std::get<T>(RolesTuple);
    }
    // ... const overload for Role()
    
    template<typename T>
    constexpr T& Attribute() {
        static_assert(HasAttribute<T>(), "Attempted to access an Attribute that does not exist on this Composition.");
        return std::get<T>(AttributesTuple);
    }
    // ... const overload for Attribute()
};


// --- EXAMPLE USAGE ---
#ifdef COMPOSITION_ENABLE_EXAMPLES

// --- 1. Define Attributes ---
struct Transform : public Attribute {
    float X = 0.0f, Y = 0.0f, Z = 0.0f;
};
struct Category : public Attribute {
    std::string Name = "Default";
    void SetName(const std::string& InName) { Name = InName; }
    const std::string& GetName() const { return Name; }
};

// --- 2. Define Roles ---
class Logger : public Role {
    // This role has no dependencies, so it implicitly uses the default
    // 'using RequiredAttributes = TypeList<>;' from the base Role struct.
public:
    explicit Logger(const std::string& DefaultContext) : Context(DefaultContext) {}
    template<typename HostType>
    void Log(const HostType& InHost, const std::string& Message) const {
        std::string CategoryName = Context;
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
    // This role OVERRIDES the default to declare a hard requirement.
    using RequiredAttributes = TypeList<Transform>;
    template<typename HostType>
    void MoveX(HostType& InHost, float DeltaX) {
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
        // FIXED: The constructor call is now clean and direct.
        : Composition(
            Logger(InName), Mover(), // Roles are passed directly
            Transform{100.0f, 0, 0}, Category{} // Attributes are passed directly
        )
    {
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
    MyPlayer.Update();
    return 0;
}

#endif // COMPOSITION_ENABLE_EXAMPLES

