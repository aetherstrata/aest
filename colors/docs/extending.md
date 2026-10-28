# Extending {#colors_extending}

Recipes for adding to `aest.colors`, inside the library or from user code.
@ref colors_architecture explains the design they build on.

## Adding a color space

1. **Define the type.** Derive from aest::colors::ColorBase (CRTP), or write
   any type satisfying aest::colors::ColorType. Give it:
   - a `static constexpr std::string_view TYPE`, printed by `std::format`;
   - a constructor taking `(ch1, ch2, ch3, alpha)` that brings each channel
     into the range of the space (clamp, wrap a hue, ...);
   - named accessors for its channels.
   Inherit aest::colors::ColorArithmetic only if adding and multiplying the
   channels is physically meaningful.
2. **Place it in the conversion tree.** Pick the parent closest to the space,
   the one with the simplest exact conversion, and specialize
   aest::colors::ParentSpace:
   ```cpp
   template <>
   struct aest::colors::ParentSpace<MySpace>
   {
       using type = aest::colors::LinearRGB;
   };
   ```
3. **Convert to and from the parent.** Specialize aest::colors::Converter
   both ways, each with a static `convert`. Mark it `constexpr` only if it
   avoids `<cmath>`. convert() now reaches every other space.
4. **Add shortcuts** where the path through the tree loses precision or
   speed, as Converter specializations for that pair.
5. **Support operations** by specializing their traits: aest::colors::Lightness
   if the space has a lightness axis, aest::colors::Interpolation if its
   channels blend, aest::colors::Euclidean if they are Cartesian coordinates.

Specializations must be visible before the first conversion that uses them:
concepts such as aest::colors::ConvertibleTo are evaluated once per
translation unit, and a later specialization would not change the result.

Test the round trip through the parent, a few known values, alpha
preservation, and `ConvertibleTo` with an existing space.

## Adding an operation

1. **Write the trait.** Declare a class template over the color space, left
   undefined, holding the space-specific step as static functions, like
   aest::colors::Lightness.
2. **Write its concept**, true when the trait is specialized for a space,
   like aest::colors::LightnessSpace.
3. **Specialize the trait** for each space where the operation makes sense.
4. **Write the function** generically: the working space is the first
   template parameter, constrained by the concept, with a default; the color
   type is deduced; a trailing aest::colors::In parameter picks the space at
   the call site. Constrain it on aest::colors::ConvertibleTo both ways.
   ```cpp
   template <MySpaceConcept Space = HSL, ColorType Color>
       requires ConvertibleTo<Space, Color> && ConvertibleTo<Color, Space>
   constexpr Color my_operation(const Color& color, float amount, In<Space> space = {});
   ```
5. **Return the input's type**, converting back from the working space.

Choose the default space deliberately: changing it later changes the colors
every existing call produces, which is a breaking change.
