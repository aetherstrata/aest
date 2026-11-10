/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file NamedColors.cppm
 * @brief Partition `aest.colors:named_colors`: the CSS named colors as SRGB
 *        constants, in the aest::colors::css namespace.
 */
export module aest.colors:named_colors;

import :color;
import :gamut;
import :hex;

/**
 * @brief The CSS named colors, kept out of aest::colors so their short names
 * (`red`, `tan`, ...) do not clash with other code.
 *
 * Write `css::coral` after `using namespace aest::colors;`, or add
 * `using namespace aest::colors::css;` where bare names are wanted.
 * @see @ref colors_ref_operations "CSS Color 4"
 */
export namespace aest::colors::css
{
///@name Pink colors
///@{
constexpr SRGB mediumVioletRed = from_hex(0xC71585); ///< CSS `mediumvioletred`, `#C71585`.
constexpr SRGB deepPink        = from_hex(0xFF1493); ///< CSS `deeppink`, `#FF1493`.
constexpr SRGB paleVioletRed   = from_hex(0xDB7093); ///< CSS `palevioletred`, `#DB7093`.
constexpr SRGB hotPink         = from_hex(0xFF69B4); ///< CSS `hotpink`, `#FF69B4`.
constexpr SRGB lightPink       = from_hex(0xFFB6C1); ///< CSS `lightpink`, `#FFB6C1`.
constexpr SRGB pink            = from_hex(0xFFC0CB); ///< CSS `pink`, `#FFC0CB`.
///@}
///@name Red colors
///@{
constexpr SRGB darkRed     = from_hex(0x8B0000); ///< CSS `darkred`, `#8B0000`.
constexpr SRGB red         = from_hex(0xFF0000); ///< CSS `red`, `#FF0000`.
constexpr SRGB fireBrick   = from_hex(0xB22222); ///< CSS `firebrick`, `#B22222`.
constexpr SRGB crimson     = from_hex(0xDC143C); ///< CSS `crimson`, `#DC143C`.
constexpr SRGB indianRed   = from_hex(0xCD5C5C); ///< CSS `indianred`, `#CD5C5C`.
constexpr SRGB lightCoral  = from_hex(0xF08080); ///< CSS `lightcoral`, `#F08080`.
constexpr SRGB salmon      = from_hex(0xFA8072); ///< CSS `salmon`, `#FA8072`.
constexpr SRGB darkSalmon  = from_hex(0xE9967A); ///< CSS `darksalmon`, `#E9967A`.
constexpr SRGB lightSalmon = from_hex(0xFFA07A); ///< CSS `lightsalmon`, `#FFA07A`.
///@}
///@name Orange colors
///@{
constexpr SRGB orangeRed  = from_hex(0xFF4500); ///< CSS `orangered`, `#FF4500`.
constexpr SRGB tomato     = from_hex(0xFF6347); ///< CSS `tomato`, `#FF6347`.
constexpr SRGB darkOrange = from_hex(0xFF8C00); ///< CSS `darkorange`, `#FF8C00`.
constexpr SRGB coral      = from_hex(0xFF7F50); ///< CSS `coral`, `#FF7F50`.
constexpr SRGB orange     = from_hex(0xFFA500); ///< CSS `orange`, `#FFA500`.
///@}
///@name Yellow colors
///@{
constexpr SRGB darkKhaki     = from_hex(0xBDB76B); ///< CSS `darkkhaki`, `#BDB76B`.
constexpr SRGB gold          = from_hex(0xFFD700); ///< CSS `gold`, `#FFD700`.
constexpr SRGB khaki         = from_hex(0xF0E68C); ///< CSS `khaki`, `#F0E68C`.
constexpr SRGB peachPuff     = from_hex(0xFFDAB9); ///< CSS `peachpuff`, `#FFDAB9`.
constexpr SRGB yellow        = from_hex(0xFFFF00); ///< CSS `yellow`, `#FFFF00`.
constexpr SRGB paleGoldenrod = from_hex(0xEEE8AA); ///< CSS `palegoldenrod`, `#EEE8AA`.
constexpr SRGB moccasin      = from_hex(0xFFE4B5); ///< CSS `moccasin`, `#FFE4B5`.
constexpr SRGB papayaWhip    = from_hex(0xFFEFD5); ///< CSS `papayawhip`, `#FFEFD5`.
constexpr SRGB lightGoldenrodYellow =
    from_hex(0xFAFAD2);                           ///< CSS `lightgoldenrodyellow`, `#FAFAD2`.
constexpr SRGB lemonChiffon = from_hex(0xFFFACD); ///< CSS `lemonchiffon`, `#FFFACD`.
constexpr SRGB lightYellow  = from_hex(0xFFFFE0); ///< CSS `lightyellow`, `#FFFFE0`.
///@}
///@name Brown colors
///@{
constexpr SRGB maroon         = from_hex(0x800000); ///< CSS `maroon`, `#800000`.
constexpr SRGB brown          = from_hex(0xA52A2A); ///< CSS `brown`, `#A52A2A`.
constexpr SRGB saddleBrown    = from_hex(0x8B4513); ///< CSS `saddlebrown`, `#8B4513`.
constexpr SRGB sienna         = from_hex(0xA0522D); ///< CSS `sienna`, `#A0522D`.
constexpr SRGB chocolate      = from_hex(0xD2691E); ///< CSS `chocolate`, `#D2691E`.
constexpr SRGB darkGoldenrod  = from_hex(0xB8860B); ///< CSS `darkgoldenrod`, `#B8860B`.
constexpr SRGB peru           = from_hex(0xCD853F); ///< CSS `peru`, `#CD853F`.
constexpr SRGB rosyBrown      = from_hex(0xBC8F8F); ///< CSS `rosybrown`, `#BC8F8F`.
constexpr SRGB goldenrod      = from_hex(0xDAA520); ///< CSS `goldenrod`, `#DAA520`.
constexpr SRGB sandyBrown     = from_hex(0xF4A460); ///< CSS `sandybrown`, `#F4A460`.
constexpr SRGB tan            = from_hex(0xD2B48C); ///< CSS `tan`, `#D2B48C`.
constexpr SRGB burlywood      = from_hex(0xDEB887); ///< CSS `burlywood`, `#DEB887`.
constexpr SRGB wheat          = from_hex(0xF5DEB3); ///< CSS `wheat`, `#F5DEB3`.
constexpr SRGB navajoWhite    = from_hex(0xFFDEAD); ///< CSS `navajowhite`, `#FFDEAD`.
constexpr SRGB bisque         = from_hex(0xFFE4C4); ///< CSS `bisque`, `#FFE4C4`.
constexpr SRGB blanchedAlmond = from_hex(0xFFEBCD); ///< CSS `blanchedalmond`, `#FFEBCD`.
constexpr SRGB cornsilk       = from_hex(0xFFF8DC); ///< CSS `cornsilk`, `#FFF8DC`.
///@}
///@name Purple / Violet tones
///@{
constexpr SRGB indigo          = from_hex(0x4B0082); ///< CSS `indigo`, `#4B0082`.
constexpr SRGB purple          = from_hex(0x800080); ///< CSS `purple`, `#800080`.
constexpr SRGB darkMagenta     = from_hex(0x8B008B); ///< CSS `darkmagenta`, `#8B008B`.
constexpr SRGB darkViolet      = from_hex(0x9400D3); ///< CSS `darkviolet`, `#9400D3`.
constexpr SRGB darkSlateBlue   = from_hex(0x483D8B); ///< CSS `darkslateblue`, `#483D8B`.
constexpr SRGB blueViolet      = from_hex(0x8A2BE2); ///< CSS `blueviolet`, `#8A2BE2`.
constexpr SRGB darkOrchid      = from_hex(0x9932CC); ///< CSS `darkorchid`, `#9932CC`.
constexpr SRGB fuchsia         = from_hex(0xFF00FF); ///< CSS `fuchsia`, `#FF00FF`.
constexpr SRGB magenta         = from_hex(0xFF00FF); ///< CSS `magenta`, `#FF00FF`.
constexpr SRGB slateBlue       = from_hex(0x6A5ACD); ///< CSS `slateblue`, `#6A5ACD`.
constexpr SRGB mediumSlateBlue = from_hex(0x7B68EE); ///< CSS `mediumslateblue`, `#7B68EE`.
constexpr SRGB mediumOrchid    = from_hex(0xBA55D3); ///< CSS `mediumorchid`, `#BA55D3`.
constexpr SRGB mediumPurple    = from_hex(0x9370DB); ///< CSS `mediumpurple`, `#9370DB`.
constexpr SRGB orchid          = from_hex(0xDA70D6); ///< CSS `orchid`, `#DA70D6`.
constexpr SRGB violet          = from_hex(0xEE82EE); ///< CSS `violet`, `#EE82EE`.
constexpr SRGB plum            = from_hex(0xDDA0DD); ///< CSS `plum`, `#DDA0DD`.
constexpr SRGB thistle         = from_hex(0xD8BFD8); ///< CSS `thistle`, `#D8BFD8`.
constexpr SRGB lavender        = from_hex(0xE6E6FA); ///< CSS `lavender`, `#E6E6FA`.
///@}
///@name Blue tones
///@{
constexpr SRGB midnightBlue   = from_hex(0x191970); ///< CSS `midnightblue`, `#191970`.
constexpr SRGB navy           = from_hex(0x000080); ///< CSS `navy`, `#000080`.
constexpr SRGB darkBlue       = from_hex(0x00008B); ///< CSS `darkblue`, `#00008B`.
constexpr SRGB mediumBlue     = from_hex(0x0000CD); ///< CSS `mediumblue`, `#0000CD`.
constexpr SRGB blue           = from_hex(0x0000FF); ///< CSS `blue`, `#0000FF`.
constexpr SRGB royalBlue      = from_hex(0x4169E1); ///< CSS `royalblue`, `#4169E1`.
constexpr SRGB steelBlue      = from_hex(0x4682B4); ///< CSS `steelblue`, `#4682B4`.
constexpr SRGB dodgerBlue     = from_hex(0x1E90FF); ///< CSS `dodgerblue`, `#1E90FF`.
constexpr SRGB deepSkyBlue    = from_hex(0x00BFFF); ///< CSS `deepskyblue`, `#00BFFF`.
constexpr SRGB cornflowerBlue = from_hex(0x6495ED); ///< CSS `cornflowerblue`, `#6495ED`.
constexpr SRGB skyBlue        = from_hex(0x87CEEB); ///< CSS `skyblue`, `#87CEEB`.
constexpr SRGB lightSkyBlue   = from_hex(0x87CEFA); ///< CSS `lightskyblue`, `#87CEFA`.
constexpr SRGB lightSteelBlue = from_hex(0xB0C4DE); ///< CSS `lightsteelblue`, `#B0C4DE`.
constexpr SRGB lightBlue      = from_hex(0xADD8E6); ///< CSS `lightblue`, `#ADD8E6`.
constexpr SRGB powderBlue     = from_hex(0xB0E0E6); ///< CSS `powderblue`, `#B0E0E6`.
///@}
///@name Cyan tones
///@{
constexpr SRGB teal            = from_hex(0x008080); ///< CSS `teal`, `#008080`.
constexpr SRGB darkCyan        = from_hex(0x008B8B); ///< CSS `darkcyan`, `#008B8B`.
constexpr SRGB lightSeaGreen   = from_hex(0x20B2AA); ///< CSS `lightseagreen`, `#20B2AA`.
constexpr SRGB cadetBlue       = from_hex(0x5F9EA0); ///< CSS `cadetblue`, `#5F9EA0`.
constexpr SRGB darkTurquoise   = from_hex(0x00CED1); ///< CSS `darkturquoise`, `#00CED1`.
constexpr SRGB mediumTurquoise = from_hex(0x48D1CC); ///< CSS `mediumturquoise`, `#48D1CC`.
constexpr SRGB turquoise       = from_hex(0x40E0D0); ///< CSS `turquoise`, `#40E0D0`.
constexpr SRGB aqua            = from_hex(0x00FFFF); ///< CSS `aqua`, `#00FFFF`.
constexpr SRGB cyan            = from_hex(0x00FFFF); ///< CSS `cyan`, `#00FFFF`.
constexpr SRGB aquamarine      = from_hex(0x7FFFD4); ///< CSS `aquamarine`, `#7FFFD4`.
constexpr SRGB paleTurquoise   = from_hex(0xAFEEEE); ///< CSS `paleturquoise`, `#AFEEEE`.
constexpr SRGB lightCyan       = from_hex(0xE0FFFF); ///< CSS `lightcyan`, `#E0FFFF`.
///@}
///@name Green tones
///@{
constexpr SRGB darkGreen         = from_hex(0x006400); ///< CSS `darkgreen`, `#006400`.
constexpr SRGB green             = from_hex(0x008000); ///< CSS `green`, `#008000`.
constexpr SRGB darkOliveGreen    = from_hex(0x556B2F); ///< CSS `darkolivegreen`, `#556B2F`.
constexpr SRGB forestGreen       = from_hex(0x228B22); ///< CSS `forestgreen`, `#228B22`.
constexpr SRGB seaGreen          = from_hex(0x2E8B57); ///< CSS `seagreen`, `#2E8B57`.
constexpr SRGB olive             = from_hex(0x808000); ///< CSS `olive`, `#808000`.
constexpr SRGB oliveDrab         = from_hex(0x6B8E23); ///< CSS `olivedrab`, `#6B8E23`.
constexpr SRGB mediumSeaGreen    = from_hex(0x3CB371); ///< CSS `mediumseagreen`, `#3CB371`.
constexpr SRGB limeGreen         = from_hex(0x32CD32); ///< CSS `limegreen`, `#32CD32`.
constexpr SRGB lime              = from_hex(0x00FF00); ///< CSS `lime`, `#00FF00`.
constexpr SRGB springGreen       = from_hex(0x00FF7F); ///< CSS `springgreen`, `#00FF7F`.
constexpr SRGB mediumSpringGreen = from_hex(0x00FA9A); ///< CSS `mediumspringgreen`, `#00FA9A`.
constexpr SRGB darkSeaGreen      = from_hex(0x8FBC8F); ///< CSS `darkseagreen`, `#8FBC8F`.
constexpr SRGB mediumAquamarine  = from_hex(0x66CDAA); ///< CSS `mediumaquamarine`, `#66CDAA`.
constexpr SRGB yellowGreen       = from_hex(0x9ACD32); ///< CSS `yellowgreen`, `#9ACD32`.
constexpr SRGB lawnGreen         = from_hex(0x7CFC00); ///< CSS `lawngreen`, `#7CFC00`.
constexpr SRGB chartreuse        = from_hex(0x7FFF00); ///< CSS `chartreuse`, `#7FFF00`.
constexpr SRGB lightGreen        = from_hex(0x90EE90); ///< CSS `lightgreen`, `#90EE90`.
constexpr SRGB greenYellow       = from_hex(0xADFF2F); ///< CSS `greenyellow`, `#ADFF2F`.
constexpr SRGB paleGreen         = from_hex(0x98FB98); ///< CSS `palegreen`, `#98FB98`.
///@}
///@name White colors
///@{
constexpr SRGB mistyRose     = from_hex(0xFFE4E1); ///< CSS `mistyrose`, `#FFE4E1`.
constexpr SRGB antiqueWhite  = from_hex(0xFAEBD7); ///< CSS `antiquewhite`, `#FAEBD7`.
constexpr SRGB linen         = from_hex(0xFAF0E6); ///< CSS `linen`, `#FAF0E6`.
constexpr SRGB beige         = from_hex(0xF5F5DC); ///< CSS `beige`, `#F5F5DC`.
constexpr SRGB whiteSmoke    = from_hex(0xF5F5F5); ///< CSS `whitesmoke`, `#F5F5F5`.
constexpr SRGB lavenderBlush = from_hex(0xFFF0F5); ///< CSS `lavenderblush`, `#FFF0F5`.
constexpr SRGB oldLace       = from_hex(0xFDF5E6); ///< CSS `oldlace`, `#FDF5E6`.
constexpr SRGB aliceBlue     = from_hex(0xF0F8FF); ///< CSS `aliceblue`, `#F0F8FF`.
constexpr SRGB seashell      = from_hex(0xFFF5EE); ///< CSS `seashell`, `#FFF5EE`.
constexpr SRGB ghostWhite    = from_hex(0xF8F8FF); ///< CSS `ghostwhite`, `#F8F8FF`.
constexpr SRGB honeydew      = from_hex(0xF0FFF0); ///< CSS `honeydew`, `#F0FFF0`.
constexpr SRGB floralWhite   = from_hex(0xFFFAF0); ///< CSS `floralwhite`, `#FFFAF0`.
constexpr SRGB azure         = from_hex(0xF0FFFF); ///< CSS `azure`, `#F0FFFF`.
constexpr SRGB mintCream     = from_hex(0xF5FFFA); ///< CSS `mintcream`, `#F5FFFA`.
constexpr SRGB snow          = from_hex(0xFFFAFA); ///< CSS `snow`, `#FFFAFA`.
constexpr SRGB ivory         = from_hex(0xFFFFF0); ///< CSS `ivory`, `#FFFFF0`.
constexpr SRGB white         = from_hex(0xFFFFFF); ///< CSS `white`, `#FFFFFF`.
///@}
///@name Gray and black colors
///@{
constexpr SRGB black          = from_hex(0x000000); ///< CSS `black`, `#000000`.
constexpr SRGB darkSlateGray  = from_hex(0x2F4F4F); ///< CSS `darkslategray`, `#2F4F4F`.
constexpr SRGB dimGray        = from_hex(0x696969); ///< CSS `dimgray`, `#696969`.
constexpr SRGB slateGray      = from_hex(0x708090); ///< CSS `slategray`, `#708090`.
constexpr SRGB gray           = from_hex(0x808080); ///< CSS `gray`, `#808080`.
constexpr SRGB lightSlateGray = from_hex(0x778899); ///< CSS `lightslategray`, `#778899`.
constexpr SRGB darkGray       = from_hex(0xA9A9A9); ///< CSS `darkgray`, `#A9A9A9`.
constexpr SRGB silver         = from_hex(0xC0C0C0); ///< CSS `silver`, `#C0C0C0`.
constexpr SRGB lightGray      = from_hex(0xD3D3D3); ///< CSS `lightgray`, `#D3D3D3`.
constexpr SRGB gainsboro      = from_hex(0xDCDCDC); ///< CSS `gainsboro`, `#DCDCDC`.
///@}
} // namespace aest::colors::css
