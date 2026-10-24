/**
 * @copyright Copyright (c) 2026 Davide Pierotti, MIT License (see LICENSE.md)
 * @author Davide Pierotti
 * @file NamedColors.cppm
 * @brief Partition `aest.colors:named_colors`: the CSS named colors as RGB
 *        constants, in the aest::colors::css namespace.
 */
export module aest.colors:named_colors;

import :color;

/**
 * @brief The CSS named colors, kept out of aest::colors so their short names
 * (`red`, `tan`, ...) do not clash with other code.
 *
 * Write `css::coral` after `using namespace aest::colors;`, or add
 * `using namespace aest::colors::css;` where bare names are wanted.
 */
export namespace aest::colors::css
{
///@name Pink colors
///@{
constexpr RGB mediumVioletRed = RGB::from_hex(0xC71585); ///< CSS `mediumvioletred`, `#C71585`.
constexpr RGB deepPink        = RGB::from_hex(0xFF1493); ///< CSS `deeppink`, `#FF1493`.
constexpr RGB paleVioletRed   = RGB::from_hex(0xDB7093); ///< CSS `palevioletred`, `#DB7093`.
constexpr RGB hotPink         = RGB::from_hex(0xFF69B4); ///< CSS `hotpink`, `#FF69B4`.
constexpr RGB lightPink       = RGB::from_hex(0xFFB6C1); ///< CSS `lightpink`, `#FFB6C1`.
constexpr RGB pink            = RGB::from_hex(0xFFC0CB); ///< CSS `pink`, `#FFC0CB`.
///@}
///@name Red colors
///@{
constexpr RGB darkRed     = RGB::from_hex(0x8B0000); ///< CSS `darkred`, `#8B0000`.
constexpr RGB red         = RGB::from_hex(0xFF0000); ///< CSS `red`, `#FF0000`.
constexpr RGB fireBrick   = RGB::from_hex(0xB22222); ///< CSS `firebrick`, `#B22222`.
constexpr RGB crimson     = RGB::from_hex(0xDC143C); ///< CSS `crimson`, `#DC143C`.
constexpr RGB indianRed   = RGB::from_hex(0xCD5C5C); ///< CSS `indianred`, `#CD5C5C`.
constexpr RGB lightCoral  = RGB::from_hex(0xF08080); ///< CSS `lightcoral`, `#F08080`.
constexpr RGB salmon      = RGB::from_hex(0xFA8072); ///< CSS `salmon`, `#FA8072`.
constexpr RGB darkSalmon  = RGB::from_hex(0xE9967A); ///< CSS `darksalmon`, `#E9967A`.
constexpr RGB lightSalmon = RGB::from_hex(0xFFA07A); ///< CSS `lightsalmon`, `#FFA07A`.
///@}
///@name Orange colors
///@{
constexpr RGB orangeRed  = RGB::from_hex(0xFF4500); ///< CSS `orangered`, `#FF4500`.
constexpr RGB tomato     = RGB::from_hex(0xFF6347); ///< CSS `tomato`, `#FF6347`.
constexpr RGB darkOrange = RGB::from_hex(0xFF8C00); ///< CSS `darkorange`, `#FF8C00`.
constexpr RGB coral      = RGB::from_hex(0xFF7F50); ///< CSS `coral`, `#FF7F50`.
constexpr RGB orange     = RGB::from_hex(0xFFA500); ///< CSS `orange`, `#FFA500`.
///@}
///@name Yellow colors
///@{
constexpr RGB darkKhaki     = RGB::from_hex(0xBDB76B); ///< CSS `darkkhaki`, `#BDB76B`.
constexpr RGB gold          = RGB::from_hex(0xFFD700); ///< CSS `gold`, `#FFD700`.
constexpr RGB khaki         = RGB::from_hex(0xF0E68C); ///< CSS `khaki`, `#F0E68C`.
constexpr RGB peachPuff     = RGB::from_hex(0xFFDAB9); ///< CSS `peachpuff`, `#FFDAB9`.
constexpr RGB yellow        = RGB::from_hex(0xFFFF00); ///< CSS `yellow`, `#FFFF00`.
constexpr RGB paleGoldenrod = RGB::from_hex(0xEEE8AA); ///< CSS `palegoldenrod`, `#EEE8AA`.
constexpr RGB moccasin      = RGB::from_hex(0xFFE4B5); ///< CSS `moccasin`, `#FFE4B5`.
constexpr RGB papayaWhip    = RGB::from_hex(0xFFEFD5); ///< CSS `papayawhip`, `#FFEFD5`.
constexpr RGB lightGoldenrodYellow =
    RGB::from_hex(0xFAFAD2);                          ///< CSS `lightgoldenrodyellow`, `#FAFAD2`.
constexpr RGB lemonChiffon = RGB::from_hex(0xFFFACD); ///< CSS `lemonchiffon`, `#FFFACD`.
constexpr RGB lightYellow  = RGB::from_hex(0xFFFFE0); ///< CSS `lightyellow`, `#FFFFE0`.
///@}
///@name Brown colors
///@{
constexpr RGB maroon         = RGB::from_hex(0x800000); ///< CSS `maroon`, `#800000`.
constexpr RGB brown          = RGB::from_hex(0xA52A2A); ///< CSS `brown`, `#A52A2A`.
constexpr RGB saddleBrown    = RGB::from_hex(0x8B4513); ///< CSS `saddlebrown`, `#8B4513`.
constexpr RGB sienna         = RGB::from_hex(0xA0522D); ///< CSS `sienna`, `#A0522D`.
constexpr RGB chocolate      = RGB::from_hex(0xD2691E); ///< CSS `chocolate`, `#D2691E`.
constexpr RGB darkGoldenrod  = RGB::from_hex(0xB8860B); ///< CSS `darkgoldenrod`, `#B8860B`.
constexpr RGB peru           = RGB::from_hex(0xCD853F); ///< CSS `peru`, `#CD853F`.
constexpr RGB rosyBrown      = RGB::from_hex(0xBC8F8F); ///< CSS `rosybrown`, `#BC8F8F`.
constexpr RGB goldenrod      = RGB::from_hex(0xDAA520); ///< CSS `goldenrod`, `#DAA520`.
constexpr RGB sandyBrown     = RGB::from_hex(0xF4A460); ///< CSS `sandybrown`, `#F4A460`.
constexpr RGB tan            = RGB::from_hex(0xD2B48C); ///< CSS `tan`, `#D2B48C`.
constexpr RGB burlywood      = RGB::from_hex(0xDEB887); ///< CSS `burlywood`, `#DEB887`.
constexpr RGB wheat          = RGB::from_hex(0xF5DEB3); ///< CSS `wheat`, `#F5DEB3`.
constexpr RGB navajoWhite    = RGB::from_hex(0xFFDEAD); ///< CSS `navajowhite`, `#FFDEAD`.
constexpr RGB bisque         = RGB::from_hex(0xFFE4C4); ///< CSS `bisque`, `#FFE4C4`.
constexpr RGB blanchedAlmond = RGB::from_hex(0xFFEBCD); ///< CSS `blanchedalmond`, `#FFEBCD`.
constexpr RGB cornsilk       = RGB::from_hex(0xFFF8DC); ///< CSS `cornsilk`, `#FFF8DC`.
///@}
///@name Purple / Violet tones
///@{
constexpr RGB indigo          = RGB::from_hex(0x4B0082); ///< CSS `indigo`, `#4B0082`.
constexpr RGB purple          = RGB::from_hex(0x800080); ///< CSS `purple`, `#800080`.
constexpr RGB darkMagenta     = RGB::from_hex(0x8B008B); ///< CSS `darkmagenta`, `#8B008B`.
constexpr RGB darkViolet      = RGB::from_hex(0x9400D3); ///< CSS `darkviolet`, `#9400D3`.
constexpr RGB darkSlateBlue   = RGB::from_hex(0x483D8B); ///< CSS `darkslateblue`, `#483D8B`.
constexpr RGB blueViolet      = RGB::from_hex(0x8A2BE2); ///< CSS `blueviolet`, `#8A2BE2`.
constexpr RGB darkOrchid      = RGB::from_hex(0x9932CC); ///< CSS `darkorchid`, `#9932CC`.
constexpr RGB fuchsia         = RGB::from_hex(0xFF00FF); ///< CSS `fuchsia`, `#FF00FF`.
constexpr RGB magenta         = RGB::from_hex(0xFF00FF); ///< CSS `magenta`, `#FF00FF`.
constexpr RGB slateBlue       = RGB::from_hex(0x6A5ACD); ///< CSS `slateblue`, `#6A5ACD`.
constexpr RGB mediumSlateBlue = RGB::from_hex(0x7B68EE); ///< CSS `mediumslateblue`, `#7B68EE`.
constexpr RGB mediumOrchid    = RGB::from_hex(0xBA55D3); ///< CSS `mediumorchid`, `#BA55D3`.
constexpr RGB mediumPurple    = RGB::from_hex(0x9370DB); ///< CSS `mediumpurple`, `#9370DB`.
constexpr RGB orchid          = RGB::from_hex(0xDA70D6); ///< CSS `orchid`, `#DA70D6`.
constexpr RGB violet          = RGB::from_hex(0xEE82EE); ///< CSS `violet`, `#EE82EE`.
constexpr RGB plum            = RGB::from_hex(0xDDA0DD); ///< CSS `plum`, `#DDA0DD`.
constexpr RGB thistle         = RGB::from_hex(0xD8BFD8); ///< CSS `thistle`, `#D8BFD8`.
constexpr RGB lavender        = RGB::from_hex(0xE6E6FA); ///< CSS `lavender`, `#E6E6FA`.
///@}
///@name Blue tones
///@{
constexpr RGB midnightBlue   = RGB::from_hex(0x191970); ///< CSS `midnightblue`, `#191970`.
constexpr RGB navy           = RGB::from_hex(0x000080); ///< CSS `navy`, `#000080`.
constexpr RGB darkBlue       = RGB::from_hex(0x00008B); ///< CSS `darkblue`, `#00008B`.
constexpr RGB mediumBlue     = RGB::from_hex(0x0000CD); ///< CSS `mediumblue`, `#0000CD`.
constexpr RGB blue           = RGB::from_hex(0x0000FF); ///< CSS `blue`, `#0000FF`.
constexpr RGB royalBlue      = RGB::from_hex(0x4169E1); ///< CSS `royalblue`, `#4169E1`.
constexpr RGB steelBlue      = RGB::from_hex(0x4682B4); ///< CSS `steelblue`, `#4682B4`.
constexpr RGB dodgerBlue     = RGB::from_hex(0x1E90FF); ///< CSS `dodgerblue`, `#1E90FF`.
constexpr RGB deepSkyBlue    = RGB::from_hex(0x00BFFF); ///< CSS `deepskyblue`, `#00BFFF`.
constexpr RGB cornflowerBlue = RGB::from_hex(0x6495ED); ///< CSS `cornflowerblue`, `#6495ED`.
constexpr RGB skyBlue        = RGB::from_hex(0x87CEEB); ///< CSS `skyblue`, `#87CEEB`.
constexpr RGB lightSkyBlue   = RGB::from_hex(0x87CEFA); ///< CSS `lightskyblue`, `#87CEFA`.
constexpr RGB lightSteelBlue = RGB::from_hex(0xB0C4DE); ///< CSS `lightsteelblue`, `#B0C4DE`.
constexpr RGB lightBlue      = RGB::from_hex(0xADD8E6); ///< CSS `lightblue`, `#ADD8E6`.
constexpr RGB powderBlue     = RGB::from_hex(0xB0E0E6); ///< CSS `powderblue`, `#B0E0E6`.
///@}
///@name Cyan tones
///@{
constexpr RGB teal            = RGB::from_hex(0x008080); ///< CSS `teal`, `#008080`.
constexpr RGB darkCyan        = RGB::from_hex(0x008B8B); ///< CSS `darkcyan`, `#008B8B`.
constexpr RGB lightSeaGreen   = RGB::from_hex(0x20B2AA); ///< CSS `lightseagreen`, `#20B2AA`.
constexpr RGB cadetBlue       = RGB::from_hex(0x5F9EA0); ///< CSS `cadetblue`, `#5F9EA0`.
constexpr RGB darkTurquoise   = RGB::from_hex(0x00CED1); ///< CSS `darkturquoise`, `#00CED1`.
constexpr RGB mediumTurquoise = RGB::from_hex(0x48D1CC); ///< CSS `mediumturquoise`, `#48D1CC`.
constexpr RGB turquoise       = RGB::from_hex(0x40E0D0); ///< CSS `turquoise`, `#40E0D0`.
constexpr RGB aqua            = RGB::from_hex(0x00FFFF); ///< CSS `aqua`, `#00FFFF`.
constexpr RGB cyan            = RGB::from_hex(0x00FFFF); ///< CSS `cyan`, `#00FFFF`.
constexpr RGB aquamarine      = RGB::from_hex(0x7FFFD4); ///< CSS `aquamarine`, `#7FFFD4`.
constexpr RGB paleTurquoise   = RGB::from_hex(0xAFEEEE); ///< CSS `paleturquoise`, `#AFEEEE`.
constexpr RGB lightCyan       = RGB::from_hex(0xE0FFFF); ///< CSS `lightcyan`, `#E0FFFF`.
///@}
///@name Green tones
///@{
constexpr RGB darkGreen         = RGB::from_hex(0x006400); ///< CSS `darkgreen`, `#006400`.
constexpr RGB green             = RGB::from_hex(0x008000); ///< CSS `green`, `#008000`.
constexpr RGB darkOliveGreen    = RGB::from_hex(0x556B2F); ///< CSS `darkolivegreen`, `#556B2F`.
constexpr RGB forestGreen       = RGB::from_hex(0x228B22); ///< CSS `forestgreen`, `#228B22`.
constexpr RGB seaGreen          = RGB::from_hex(0x2E8B57); ///< CSS `seagreen`, `#2E8B57`.
constexpr RGB olive             = RGB::from_hex(0x808000); ///< CSS `olive`, `#808000`.
constexpr RGB oliveDrab         = RGB::from_hex(0x6B8E23); ///< CSS `olivedrab`, `#6B8E23`.
constexpr RGB mediumSeaGreen    = RGB::from_hex(0x3CB371); ///< CSS `mediumseagreen`, `#3CB371`.
constexpr RGB limeGreen         = RGB::from_hex(0x32CD32); ///< CSS `limegreen`, `#32CD32`.
constexpr RGB lime              = RGB::from_hex(0x00FF00); ///< CSS `lime`, `#00FF00`.
constexpr RGB springGreen       = RGB::from_hex(0x00FF7F); ///< CSS `springgreen`, `#00FF7F`.
constexpr RGB mediumSpringGreen = RGB::from_hex(0x00FA9A); ///< CSS `mediumspringgreen`, `#00FA9A`.
constexpr RGB darkSeaGreen      = RGB::from_hex(0x8FBC8F); ///< CSS `darkseagreen`, `#8FBC8F`.
constexpr RGB mediumAquamarine  = RGB::from_hex(0x66CDAA); ///< CSS `mediumaquamarine`, `#66CDAA`.
constexpr RGB yellowGreen       = RGB::from_hex(0x9ACD32); ///< CSS `yellowgreen`, `#9ACD32`.
constexpr RGB lawnGreen         = RGB::from_hex(0x7CFC00); ///< CSS `lawngreen`, `#7CFC00`.
constexpr RGB chartreuse        = RGB::from_hex(0x7FFF00); ///< CSS `chartreuse`, `#7FFF00`.
constexpr RGB lightGreen        = RGB::from_hex(0x90EE90); ///< CSS `lightgreen`, `#90EE90`.
constexpr RGB greenYellow       = RGB::from_hex(0xADFF2F); ///< CSS `greenyellow`, `#ADFF2F`.
constexpr RGB paleGreen         = RGB::from_hex(0x98FB98); ///< CSS `palegreen`, `#98FB98`.
///@}
///@name White colors
///@{
constexpr RGB mistyRose     = RGB::from_hex(0xFFE4E1); ///< CSS `mistyrose`, `#FFE4E1`.
constexpr RGB antiqueWhite  = RGB::from_hex(0xFAEBD7); ///< CSS `antiquewhite`, `#FAEBD7`.
constexpr RGB linen         = RGB::from_hex(0xFAF0E6); ///< CSS `linen`, `#FAF0E6`.
constexpr RGB beige         = RGB::from_hex(0xF5F5DC); ///< CSS `beige`, `#F5F5DC`.
constexpr RGB whiteSmoke    = RGB::from_hex(0xF5F5F5); ///< CSS `whitesmoke`, `#F5F5F5`.
constexpr RGB lavenderBlush = RGB::from_hex(0xFFF0F5); ///< CSS `lavenderblush`, `#FFF0F5`.
constexpr RGB oldLace       = RGB::from_hex(0xFDF5E6); ///< CSS `oldlace`, `#FDF5E6`.
constexpr RGB aliceBlue     = RGB::from_hex(0xF0F8FF); ///< CSS `aliceblue`, `#F0F8FF`.
constexpr RGB seashell      = RGB::from_hex(0xFFF5EE); ///< CSS `seashell`, `#FFF5EE`.
constexpr RGB ghostWhite    = RGB::from_hex(0xF8F8FF); ///< CSS `ghostwhite`, `#F8F8FF`.
constexpr RGB honeydew      = RGB::from_hex(0xF0FFF0); ///< CSS `honeydew`, `#F0FFF0`.
constexpr RGB floralWhite   = RGB::from_hex(0xFFFAF0); ///< CSS `floralwhite`, `#FFFAF0`.
constexpr RGB azure         = RGB::from_hex(0xF0FFFF); ///< CSS `azure`, `#F0FFFF`.
constexpr RGB mintCream     = RGB::from_hex(0xF5FFFA); ///< CSS `mintcream`, `#F5FFFA`.
constexpr RGB snow          = RGB::from_hex(0xFFFAFA); ///< CSS `snow`, `#FFFAFA`.
constexpr RGB ivory         = RGB::from_hex(0xFFFFF0); ///< CSS `ivory`, `#FFFFF0`.
constexpr RGB white         = RGB::from_hex(0xFFFFFF); ///< CSS `white`, `#FFFFFF`.
///@}
///@name Gray and black colors
///@{
constexpr RGB black          = RGB::from_hex(0x000000); ///< CSS `black`, `#000000`.
constexpr RGB darkSlateGray  = RGB::from_hex(0x2F4F4F); ///< CSS `darkslategray`, `#2F4F4F`.
constexpr RGB dimGray        = RGB::from_hex(0x696969); ///< CSS `dimgray`, `#696969`.
constexpr RGB slateGray      = RGB::from_hex(0x708090); ///< CSS `slategray`, `#708090`.
constexpr RGB gray           = RGB::from_hex(0x808080); ///< CSS `gray`, `#808080`.
constexpr RGB lightSlateGray = RGB::from_hex(0x778899); ///< CSS `lightslategray`, `#778899`.
constexpr RGB darkGray       = RGB::from_hex(0xA9A9A9); ///< CSS `darkgray`, `#A9A9A9`.
constexpr RGB silver         = RGB::from_hex(0xC0C0C0); ///< CSS `silver`, `#C0C0C0`.
constexpr RGB lightGray      = RGB::from_hex(0xD3D3D3); ///< CSS `lightgray`, `#D3D3D3`.
constexpr RGB gainsboro      = RGB::from_hex(0xDCDCDC); ///< CSS `gainsboro`, `#DCDCDC`.
///@}
} // namespace aest::colors::css
