package edu.stanford.nlp.process;

// Stanford English Tokenizer -- a deterministic, fast high-quality tokenizer
// Copyright (c) 2002-2009 The Board of Trustees of
// The Leland Stanford Junior University. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// For more information, bug reports, fixes, contact:
//    Christopher Manning
//    Dept of Computer Science, Gates 1A
//    Stanford CA 94305-9010
//    USA
//    java-nlp-support@lists.stanford.edu
//    http://nlp.stanford.edu/software/


import java.io.Reader;
import java.util.logging.Logger;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;
import java.util.Set;

import edu.stanford.nlp.util.StringUtils;
import edu.stanford.nlp.ling.CoreLabel;
import edu.stanford.nlp.process.CoreLabelTokenFactory;
import edu.stanford.nlp.ling.CoreAnnotations.AfterAnnotation;
import edu.stanford.nlp.ling.CoreAnnotations.BeforeAnnotation;
import edu.stanford.nlp.ling.CoreAnnotations.CurrentAnnotation;


/** Provides a tokenizer or lexer that does a pretty good job at
 *  deterministically tokenizing English according to Penn Treebank conventions.
 *  The class is a scanner generated by
 *  <a href="http://www.jflex.de/">JFlex</a> (1.4.3) from the specification
 *  file
 *  <code>PTBLexer.flex</code>.  As well as copying what is in the Treebank,
 *  it now contains some extensions to deal with modern text and encoding
 *  issues, such as recognizing URLs and common Unicode characters, and a
 *  variety of options for doing or suppressing certain normalizations.
 *  Although they shouldn't really be there, it also interprets certain of the
 *  characters between U+0080 and U+009F as Windows CP1252 characters.
 *  <p>
 *  <i>Fine points:</i> Output normalized tokens should not contain spaces,
 *  providing the normalizeSpace option is true.  The space will be turned
 *  into a non-breaking space (U+00A0). Otherwise, they can appear in
 *  a couple of token classes (phone numbers, fractions).
 *  The original
 *  PTB tokenization (messy) standard also escapes certain other characters,
 *  such as * and /, and normalizes things like " to `` or ''.  By default,
 *  this tokenizer now doesn't do any of those things.  However, you can turn
 *  it on by using the ptb3Escaping flag, or, parts of it or unicode
 *  character alternatives with different options.  You can also build an
 *  invertible tokenizer, with which you can still access the original
 *  character sequence and the non-token whitespace around it in a CoreLabel.
 *  And you can ask for newlines to be tokenized.
 *  <p>
 *  <i>Implementation note:</i> The scanner is caseless, but note, if adding
 *  or changing regexps, that caseless does not expand inside character
 *  classes.  From the manual: "The %caseless option does not change the
 *  matched text and does not effect character classes. So [a] still only
 *  matches the character a and not A, too."  Note that some character
 *  classes still deliberately don't have both cases, so the scanner's
 *  operation isn't completely case-independent, though it mostly is.
 *  <p>
 *  <i>Implementation note:</i> This Java class is automatically generated
 *  from PTBLexer.flex using jflex.  DO NOT EDIT THE JAVA SOURCE.  This file
 *  has now been updated for JFlex 1.4.2+.  (This required code changes: this
 *  version only works right with JFlex 1.4.2+; the previous version only works
 *  right with JFlex 1.4.1.)
 *
 *  @author Tim Grow
 *  @author Christopher Manning
 *  @author Jenny Finkel
 */

%%

%class PTBLexer
%unicode
%function next
%type Object
%char
%caseless

%{

  /**
   * Constructs a new PTBLexer.  You specify the type of result tokens with a
   * LexedTokenFactory, and can specify the treatment of tokens by boolean
   * options given in a comma separated String
   * (e.g., "invertible,normalizeParentheses=true").
   * If the String is <code>null</code> or empty, you get no normalization
   * of tokens turned on (i.e., you get ptb3Escaping=false).  If you want
   * the traditional PTB3 behavior, you should pass in the String
   * "ptb3Escaping".  The known option names are:
   * <ol>
   * <li>invertible: Store enough information about the original form of the
   *     token and the whitespace around it that a list of tokens can be
   *     faithfully converted back to the original String.  Valid only if the
   *     LexedTokenFactory is an instance of CoreLabelTokenFactory.  The
   *     keys used in it are WordAnnotation for the tokenized form,
   *     CurrentAnnotation for the original string, BeforeAnnotation and
   *     AfterAnnotation for the whitespace before and after a token, and
   *     perhaps BeginPositionAnnotation and EndPositionAnnotation to record
   *     token begin/after end offsets, if they were specified to be recorded
   *     in TokenFactory construction.  (Like the String class, begin and end
   *     are done so end - begin gives the token length.)
   * <li>tokenizeNLs: Whether end-of-lines should become tokens (or just
   *     be treated as part of whitespace)
   * <li>ptb3Escaping: Enable all traditional PTB3 token transforms
   *     (like -LRB-, -RRB-).  This is a macro flag that sets or clears all the
   *     options below.
   * <li>americanize: Whether to rewrite common British English spellings
   *     as American English spellings
   * <li>normalizeSpace: Whether any spaces in tokens (phone numbers, fractions
   *     get turned into U+00A0 (non-breaking space).  It's dangerous to turn
   *     this off for most of our Stanford NLP software, which assumes no
   *     spaces in tokens.
   * <li>normalizeAmpersandEntity: Whether to map the XML &amp;amp; to an
   *      ampersand
   * <li>normalizeCurrency: Whether to do some awful lossy currency mappings
   *     to turn common currency characters into $, #, or "cents", reflecting
   *     the fact that nothing else appears in the old PTB3 WSJ.  (No Euro!)
   * <li>normalizeFractions: Whether to map certain common composed
   *     fraction characters to spelled out letter forms like "1/2"
   * <li>normalizeParentheses: Whether to map round parentheses to -LRB-,
   *     -RRB-, as in the Penn Treebank
   * <li>normalizeOtherBrackets: Whether to map other common bracket characters
   *     to -LCB-, -LRB-, -RCB-, -RRB-, roughly as in the Penn Treebank
   * <li>asciiQuotes Whether to map quote characters to the traditional ' and "
   * <li>latexQuotes: Whether to map to ``, `, ', '' for quotes, as in Latex
   *     and the PTB3 WSJ (though this is now heavily frowned on in Unicode).
   *     If true, this takes precedence over the setting of unicodeQuotes;
   *     if both are false, no mapping is done.
   * <li>unicodeQuotes: Whether to map quotes to the range U+2018 to U+201D,
   *     the preferred unicode encoding of single and double quotes.
   * <li>ptb3Ellipsis: Whether to map ellipses to ..., the old PTB3 WSJ coding
   *     of an ellipsis. If true, this takes precedence over the setting of
   *     unicodeEllipsis; if both are false, no mapping is done.
   * <li>unicodeEllipsis: Whether to map dot and optional space sequences to
   *     U+2026, the Unicode ellipsis character
   * <li>ptb3Dashes: Whether to turn various dash characters into "--",
   *     the dominant encoding of dashes in the PTB3 WSJ
   * <li>escapeForwardSlashAsterisk: Whether to put a backslash escape in front
   *     of / and * as the old PTB3 WSJ does for some reason (something to do
   *     with Lisp readers??).
   * </ol>
   *
   * @param r The Reader to tokenize text from
   * @param tf The LexedTokenFactory that will be invoked to convert
   *    each substring extracted by the lexer into some kind of Object
   *    (such as a Word or CoreLabel).
   */
  public PTBLexer(Reader r, LexedTokenFactory<?> tf, String options) {
    this(r);
    this.tokenFactory = tf;
    if (options == null) {
      options = "";
    }
    Properties prop = StringUtils.stringToProperties(options);
    Set<Map.Entry<Object,Object>> props = prop.entrySet();
    for (Map.Entry<Object,Object> item : props) {
      String key = (String) item.getKey();
      boolean val = Boolean.valueOf((String) item.getValue());
      if ("".equals(key)) {
        // allow an empty item
      } else if ("invertible".equals(key)) {
	invertible = val;
      } else if ("tokenizeNLs".equals(key)) {
	tokenizeNLs = val;
      } else if ("ptb3Escaping".equals(key)) {
	americanize = val;
	normalizeSpace = val;
        normalizeAmpersandEntity = val;
	normalizeCurrency = val;
        normalizeFractions = val;
	normalizeParentheses = val;
	normalizeOtherBrackets = val;
	latexQuotes = val;
	unicodeQuotes = val;
        asciiQuotes = val;
	ptb3Ellipsis = val;
	unicodeEllipsis = val;
	ptb3Dashes = val;
	escapeForwardSlashAsterisk = val;
      } else if ("americanize".equals(key)) {
	americanize = val;
      } else if ("normalizeSpace".equals(key)) {
	normalizeSpace = val;
      } else if ("normalizeAmpersandEntity".equals(key)) {
	normalizeAmpersandEntity = val;
      } else if ("normalizeCurrency".equals(key)) {
	normalizeCurrency = val;
      } else if ("normalizeFractions".equals(key)) {
	normalizeFractions = val;
      } else if ("normalizeParentheses".equals(key)) {
	normalizeParentheses = val;
      } else if ("normalizeOtherBrackets".equals(key)) {
	normalizeOtherBrackets = val;
      } else if ("latexQuotes".equals(key)) {
	latexQuotes = val;
      } else if ("unicodeQuotes".equals(key)) {
	unicodeQuotes = val;
        if (val) {
          latexQuotes = false; // need to override default
        }
      } else if ("asciiQuotes".equals(key)) {
	asciiQuotes = val;
        if (val) {
          latexQuotes = false; // need to override default
          unicodeQuotes = false;
        }
      } else if ("ptb3Ellipsis".equals(key)) {
	ptb3Ellipsis = val;
      } else if ("unicodeEllipsis".equals(key)) {
	unicodeEllipsis = val;
      } else if ("ptb3Dashes".equals(key)) {
	ptb3Dashes = val;
      } else if ("escapeForwardSlashAsterisk".equals(key)) {
	escapeForwardSlashAsterisk = val;
      } else {
	throw new IllegalArgumentException("PTBLexer: Invalid options key in constructor: " + key);
      }
    }
    this.seenUntokenizableCharacter = false;
    if (invertible) {
      if ( ! (tf instanceof CoreLabelTokenFactory)) {
        throw new IllegalArgumentException("PTBLexer: the invertible option requires a CoreLabelTokenFactory");
      }
      prevWord = (CoreLabel) tf.makeToken("", 0, 0);
    }
  }


  private static final Logger LOGGER = Logger.getLogger(PTBLexer.class.getName());

  private LexedTokenFactory<?> tokenFactory;
  private CoreLabel prevWord;
  private boolean seenUntokenizableCharacter;

  /* Flags begin with historical ptb3Escaping behavior */
  private boolean invertible;
  private boolean tokenizeNLs;
  private boolean americanize = true;
  private boolean normalizeSpace = true;
  private boolean normalizeAmpersandEntity = true;
  private boolean normalizeCurrency = true;
  private boolean normalizeFractions = true;
  private boolean normalizeParentheses = true;
  private boolean normalizeOtherBrackets = true;
  private boolean latexQuotes = true;
  private boolean unicodeQuotes;
  private boolean asciiQuotes;
  private boolean ptb3Ellipsis = true;
  private boolean unicodeEllipsis;
  private boolean ptb3Dashes = true;
  private boolean escapeForwardSlashAsterisk = true;

  /*
   * This has now been extended to cover the main Windows CP1252 characters,
   * at either their correct Unicode codepoints, or in their invalid
   * positions as 8 bit chars inside the iso-8859 control region.
   *
   * ellipsis  	85  	0133  	2026  	8230
   * single quote curly starting 	91 	0145 	2018 	8216
   * single quote curly ending 	92 	0146 	2019 	8217
   * double quote curly starting 	93 	0147 	201C 	8220
   * double quote curly ending 	94 	0148 	201D 	8221
   * en dash  	96  	0150  	2013  	8211
   * em dash  	97  	0151  	2014  	8212
   */

  public static final String opendblquote = "``";
  public static final String closedblquote = "''";
  public static final String openparen = "-LRB-";
  public static final String closeparen = "-RRB-";
  public static final String openbrace = "-LCB-";
  public static final String closebrace = "-RCB-";
  public static final String ptbmdash = "--";
  public static final String ptb3EllipsisStr = "...";
  public static final String unicodeEllipsisStr = "\u2026";
  /** For tokenizing carriage returns.  (JS) */
  public static final String NEWLINE_TOKEN = "*NL*";


  private Object normalizeFractions(final String in) {
    String out = in;
    if (normalizeFractions) {
      if (escapeForwardSlashAsterisk) {
        out = out.replaceAll("\u00BC", "1\\/4");
        out = out.replaceAll("\u00BD", "1\\/2");
        out = out.replaceAll("\u00BE", "3\\/4");
        out = out.replaceAll("\u2153", "1\\/3");
        out = out.replaceAll("\u2153", "2\\/3");
     } else {
        out = out.replaceAll("\u00BC", "1/4");
        out = out.replaceAll("\u00BD", "1/2");
        out = out.replaceAll("\u00BE", "3/4");
        out = out.replaceAll("\u2153", "1/3");
        out = out.replaceAll("\u2153", "2/3");
      }
    }
    return getNext(out, in);
  }

  private static String normalizeCurrency(String in) {
    String s1 = in;
    s1 = s1.replaceAll("\u00A2", "cents");
    s1 = s1.replaceAll("\u00A3", "#");  // historically used for pound in PTB3
    s1 = s1.replaceAll("[\u0080\u00A4\u20A0\u20AC]", "$");  // Euro (ECU, generic currency)  -- no good translation!
    return s1;
  }

  private static String latexQuotes(String in, boolean probablyLeft) {
    String s1 = in;
    if (probablyLeft) {
      s1 = s1.replaceAll("&apos;|'", "`");
      s1 = s1.replaceAll("\"|&quot;", "``");
    } else {
      s1 = s1.replaceAll("&apos;|'", "'");
      s1 = s1.replaceAll("\"|&quot;", "''");
    }
    s1 = s1.replaceAll("[\u0091\u2018\u2039]", "`");
    s1 = s1.replaceAll("[\u0092\u2019\u203A]", "'");
    s1 = s1.replaceAll("[\u0093\u201C\u00AB]", "``");
    s1 = s1.replaceAll("[\u0094\u201D\u00BB]", "''");
    return s1;
  }

  private static String asciiQuotes(String in, boolean probablyLeft) {
    String s1 = in;
    s1 = s1.replaceAll("&apos;|[\u0091\u2018\u0092\u2019\u201A\u2039\u203A']", "'");
    s1 = s1.replaceAll("&quot;|[\u0093\u201C\u0094\u201D\u201E\u00AB\u00BB\"]", "\"");
    return s1;
  }

  private static String unicodeQuotes(String in, boolean probablyLeft) {
    String s1 = in;
    if (probablyLeft) {
      s1 = s1.replaceAll("&apos;|'", "\u2018");
      s1 = s1.replaceAll("\"|&quot;", "\u201c");
    } else {
      s1 = s1.replaceAll("&apos;|'", "\u2019");
      s1 = s1.replaceAll("\"|&quot;", "\u201d");
    }
    s1 = s1.replaceAll("[\u0091\u2018]", "\u2018");
    s1 = s1.replaceAll("[\u0092\u2019]", "\u2019");
    s1 = s1.replaceAll("[\u0093\u201C]", "\u201c");
    s1 = s1.replaceAll("[\u0094\u201D]", "\u201d");
    return s1;
  }

  private Object handleQuotes(String tok, boolean probablyLeft) {
    String normTok;
    if (latexQuotes) {
      normTok = latexQuotes(tok, probablyLeft);
    } else if (unicodeQuotes) {
      normTok = unicodeQuotes(tok, probablyLeft);
    } else if (asciiQuotes) {
      normTok = asciiQuotes(tok, probablyLeft);
    } else {
      normTok = tok;
    }
    return getNext(normTok, tok);
  }

  private Object handleEllipsis(final String tok) {
    if (ptb3Ellipsis) {
      return getNext(ptb3EllipsisStr, tok);
    } else if (unicodeEllipsis) {
      return getNext(unicodeEllipsisStr, tok);
    } else {
      return getNext(tok, tok);
    }
  }

  /** This quotes a character with a backslash, but doesn't do it
   *  if the character is already preceded by a backslash.
   */
  private static String delimit (String s, char c) {
    int i = s.indexOf(c);
    while (i != -1) {
      if (i == 0 || s.charAt(i - 1) != '\\') {
        s = s.substring(0, i) + "\\" + s.substring(i);
        i = s.indexOf(c, i + 2);
      } else {
        i = s.indexOf(c, i + 1);
      }
    }
    return s;
  }

  private static String normalizeAmp(final String in) {
    return in.replaceAll("(?i:&amp;)", "&");
  }

  private Object getNext() {
    return getNext(yytext(), yytext());
  }

  /** Make the next token.
   *  @param txt What the token should be
   *  @param current The original String that got transformed into txt
   */
  private Object getNext(String txt, String current) {
    if (invertible) {
      CoreLabel word = (CoreLabel) tokenFactory.makeToken(txt, yychar, yylength());
      word.set(CurrentAnnotation.class, current);
      word.set(BeforeAnnotation.class, prevWord.getString(AfterAnnotation.class));
      prevWord = word;
      return word;
    } else {
      return tokenFactory.makeToken(txt, yychar, yylength());
   }
  }


%}

SGML = <\/?[A-Za-z!][^>]*>
SPMDASH = &(MD|mdash|ndash);|[\u0096\u0097\u2013\u2014\u2015]
SPAMP = &amp;
SPPUNC = &(HT|TL|UR|LR|QC|QL|QR|odq|cdq|lt|gt|#[0-9]+);
SPLET = &[aeiouAEIOU](acute|grave|uml);
SPACE = [ \t\u00A0]+
SPACENL = [ \u00A0\t\r\n]+
SENTEND = [ \t\n][ \t\n]+|[ \t\n]+([A-Z]|{SGML})
DIGIT = [:digit:]
DATE = {DIGIT}{1,2}[\-\/]{DIGIT}{1,2}[\-\/]{DIGIT}{2,4}
NUM = {DIGIT}+|{DIGIT}*([.:,]{DIGIT}+)+
/* Now don't allow bracketed negative numbers!  They have too many uses (e.g.,
   years or times in parentheses), and having them in tokens messes up
   treebank parsing.
   NUMBER = [\-+]?{NUM}|\({NUM}\) */
NUMBER = [\-+]?{NUM}
/* Constrain fraction to only match likely fractions */
FRAC = ({DIGIT}{1,4}[- \u00A0])?{DIGIT}{1,4}(\\?\/|\u2044){DIGIT}{1,4}
FRAC2 = [\u00BC\u00BD\u00BE\u2153\u2154]
DOLSIGN = ([A-Z]*\$|#)
/* These are cent and pound sign, euro and euro */
DOLSIGN2 = [\u00A2\u00A3\u00A4\u0080\u20A0\u20AC]
/* not used DOLLAR	{DOLSIGN}[ \t]*{NUMBER}  */
/* |\( ?{NUMBER} ?\))	 # is for pound signs */
WORD = ([:letter:]|{SPLET})+
/* The $ was for things like New$ */
/* WAS: only keep hyphens with short one side like co-ed */
/* But treebank just allows hyphenated things as words! */
THING = ([dDoOlL]{APOS}[A-Za-z0-9])?[A-Za-z0-9]+([_-]([dDoOlL]{APOS}[A-Za-z0-9])?[A-Za-z0-9]+)*
THINGA = [A-Z]+(([+&]|{SPAMP})[A-Z]+)+
THING3 = [A-Za-z0-9]+(-[A-Za-z]+){0,2}(\\?\/[A-Za-z0-9]+(-[A-Za-z]+){0,2}){1,2}
APOS = ['\u0092\u2019]|&apos;
HTHING = [A-Za-z0-9][A-Za-z0-9.,]*(-([A-Za-z0-9]+|{ACRO}\.))+
REDAUX = {APOS}([msdMSD]|re|ve|ll)
/* For things that will have n't on the end. They can't end in 'n' */
SWORD = [A-Za-z]*[A-MO-Za-mo-z]
SREDAUX = n{APOS}t
/* Tokens you want but already okay: C'mon 'n' '[2-9]0s '[eE]m 'till?
   [Yy]'all 'Cause Shi'ite B'Gosh o'clock.  Here now only need apostrophe
   final words. */
APOWORD = {APOS}n{APOS}?|[lLdDjJ]'|Dunkin{APOS}|somethin{APOS}|ol{APOS}|{APOS}em|C{APOS}mon|{APOS}[2-9]0s|{APOS}till?|o{APOS}clock|[A-Za-z][a-z]*[aeiou]{APOS}[aeiou][a-z]*|N'[a-z]+|{APOS}cause
FULLURL = https?:\/\/[^ \t\n\f\r\"<>|()]+[^ \t\n\f\r\"<>|.!?(){},-]
LIKELYURL = ((www\.([^ \t\n\f\r\"<>|.!?(){},]+\.)+[a-zA-Z]{2,4})|(([^ \t\n\f\r\"`'<>|.!?(){},-_$]+\.)+(com|net|org|edu)))(\/[^ \t\n\f\r\"<>|()]+[^ \t\n\f\r\"<>|.!?(){},-])?
EMAIL = [a-zA-Z0-9][^ \t\n\f\r\"<>|()\u00A0]*@([^ \t\n\f\r\"<>|().\u00A0]+\.)+[a-zA-Z]{2,4}

/* Abbreviations - induced from 1987 WSJ by hand */
ABMONTH = Jan|Feb|Mar|Apr|Jun|Jul|Aug|Sep|Sept|Oct|Nov|Dec
/* Jun and Jul barely occur, but don't seem dangerous */
ABDAYS = Mon|Tue|Tues|Wed|Thu|Thurs|Fri
/* In caseless, |a\.m|p\.m handled as ACRO, and this is better as can often
   be followed by capitalized. */
/* Sat. and Sun. barely occur and can easily lead to errors, so we omit them */
/* Ma. isn't included as too many errors, and most sources use Mass. etc. */
ABSTATE = Calif|Mass|Conn|Fla|Ill|Mich|Pa|Va|Ariz|Tenn|Mo|Md|Wis|Minn|Ind|Okla|Wash|Kan|Ore|Ga|Colo|Ky|Del|Ala|La|Nev|Neb|Ark|Miss|Vt|Wyo|Tex
ACRO = [A-Za-z](\.[A-Za-z])+|(Canada|Sino|Korean|EU|Japan|non)-U\.S|U\.S\.-(U\.K|U\.S\.S\.R)
ABTITLE = Mr|Mrs|Ms|Miss|Drs?|Profs?|Sens?|Reps?|Lt|Col|Gen|Messrs|Govs?|Adm|Rev|Maj|Sgt|Cpl|Pvt|Mt|Capt|Ste?|Ave|Pres|Lieut|Hon
ABPTIT = Jr|Sr|Bros|Ph\.D
ABCOMP = Inc|Cos?|Corp|Pty|Ltd|Plc|Bancorp|Dept|Mfg|Bhd|Assn|Univ
/* Don't included fl. oz. since Oz turns up too much in caseless tokenizer. */
ABNUM = Nos?|Prop|Ph|tel|est|ext|sq|ft
/* p used to be in ABNUM list, but it can't be any more, since the lexer
   is now caseless.  We don't want to have it recognized for P.  Both
   p. and P. are now under ABBREV4. ABLIST also went away as no-op [a-e] */
/* est. is "estimated" -- common in some financial contexts. ext. is extension */
/* ABBREV1 abbreviations are normally followed by lower case words.  If
   they're followed by an uppercase one, we assume there is also a
   sentence boundary */
ABBREV3 = {ABMONTH}|{ABDAYS}|{ABSTATE}|{ABCOMP}|{ABNUM}|{ABPTIT}|etc
ABBREV1 = {ABBREV3}\.


/* ABRREV2 abbreviations are normally followed by an upper case word.  We
   assume they aren't used sentence finally */
/* ACRO Is a bad case -- can go either way! */
ABBREV4	= [A-Za-z]|{ABTITLE}|vs|Alex|Wm|Jos|Cie|a\.k\.a|TREAS|{ACRO}
ABBREV2	= {ABBREV4}\.
/* Cie. is used by French companies sometimes before and sometimes at end as in English Co.  But we treat as allowed to have Capital following without being sentence end.  Cia. is used inSpanish/South American company abbreviations, which come before the company name, but we exclude that and lose, because in a caseless segmenter, it's too confusable with CIA. */
/* in the WSJ Alex. is generally an abbreviation for Alex. Brown, brokers! */
/* Added Wm. for William and Jos. for Joseph */
/* In tables: Mkt. for market Div. for division of company, Chg., Yr.: year */


PHONE = (\([0-9]{3}\)[ \u00A0]?|[0-9]{3}[\- \u00A0])[0-9]{3}[\- \u00A0][0-9]{4}
OPBRAC = [<\[]
CLBRAC = [>\]]
HYPHENS = \-+
LDOTS = \.{3,5}|(\.[ \u00A0]){2,4}\.|[\u0085\u2026]
ATS = @+
UNDS = _+
ASTS = \*+|(\\\*){1,3}
HASHES = #+
FNMARKS = {ATS}|{HASHES}|{UNDS}
INSENTP =[,;:]
QUOTES =`|{APOS}|``|''|[\u2018\u2019\u201C\u201D\u0091\u0092\u0093\u0094\u201A\u201E\u2039\u203A\u00AB\u00BB]{1,2}
DBLQUOT = \"|&quot;
TBSPEC = -(RRB|LRB|RCB|LCB|RSB|LSB)-|C\.D\.s|M'Bow|pro-|anti-|S&P-500|cont'd\.?|B'Gosh|S&Ls|N'Ko|'twas|nor'easter|Ha'aretz
TBSPEC2 = {APOS}[0-9][0-9]

%%

cannot			{ yypushback(3) ; return getNext(); }
{SGML}			{ return getNext(); }
{SPMDASH}		{ if (ptb3Dashes) {
                            return getNext(ptbmdash, yytext()); }
                          else {
                            return getNext();
                          }
                        }
{SPAMP}			{ if (normalizeAmpersandEntity) {
                            return getNext("&", yytext()); }
                          else {
                            return getNext();
                          }
                        }
{SPPUNC}		{ return getNext(); }
{WORD}/{REDAUX}		{ return getNext(); }
/* {SWORD}/{SREDAUX}	{ yypushback(1); return getNext(); } */
{SWORD}/{SREDAUX}	{ return getNext(); }
{WORD}			{ if (americanize) {
                            return getNext(Americanize.americanize(yytext()), yytext()); }
                          else {
                            return getNext();
                          }
                        }
{APOWORD}		{ return getNext(); }
{FULLURL}		{ return getNext(); }
{LIKELYURL}	        { return getNext(); }
{EMAIL}			{ return getNext(); }
{REDAUX}/[^A-Za-z]	{ return handleQuotes(yytext(), false);
                        }
{SREDAUX}		{ return handleQuotes(yytext(), false);
                        }
{DATE}			{ return getNext(); }
{NUMBER}		{ return getNext(); }
{FRAC}			{ String txt = yytext();
			  if (escapeForwardSlashAsterisk) {
			    txt = delimit(txt, '/');
                          }
			  if (normalizeSpace) {
			     txt = txt.replaceAll(" ", "\u00A0"); // change to non-breaking space
			  }
			  return getNext(txt, yytext());
                        }
{FRAC2}			{ return normalizeFractions(yytext()); }
{TBSPEC}		{ return getNext(); }
{THING3}		{ if (escapeForwardSlashAsterisk) {
                            return getNext(delimit(yytext(), '/'), yytext());
                          } else {
                            return getNext();
                          }
                        }
{DOLSIGN}		{ return getNext(); }
{DOLSIGN2}		{ if (normalizeCurrency) {
                            return getNext(normalizeCurrency(yytext()), yytext()); }
                          else {
                            return getNext();
                          }
                        }
{ABBREV1}/{SENTEND}	{ String s = yytext();
			  yypushback(1);  // return a period for next time
	                  return getNext(s, yytext()); }
{ABBREV1}		{ return getNext(); }
{ABBREV2}		{ return getNext(); }
{ABBREV4}/{SPACE}	{ return getNext(); }
{ACRO}/{SPACENL}	{ return getNext(); }
{TBSPEC2}/{SPACENL}	{ return getNext(); }
{WORD}\./{INSENTP}	{ return getNext(); }
{PHONE}			{ String txt = yytext();
			  if (normalizeSpace) {
			    txt = txt.replaceAll(" ", "\u00A0"); // change to non-breaking space
			  }
			  return getNext(txt, yytext());
			}
{DBLQUOT}/[A-Za-z0-9$]	{ return handleQuotes(yytext(), true); }
{DBLQUOT}		{ return handleQuotes(yytext(), false); }
\+		{ return getNext(); }
%|&		{ return getNext(); }
\~|\^		{ return getNext(); }
\||\\|0x7f	{ if (invertible) {
                     prevWord.appendAfter(yytext());
                  } }
{OPBRAC}	{ if (normalizeOtherBrackets) {
                    return getNext(openparen, yytext()); }
                  else {
                    return getNext();
                  }
                }
{CLBRAC}	{ if (normalizeOtherBrackets) {
                    return getNext(closeparen, yytext()); }
                  else {
                    return getNext();
                  }
                }
\{		{ if (normalizeOtherBrackets) {
                    return getNext(openbrace, yytext()); }
                  else {
                    return getNext();
                  }
                }
\}		{ if (normalizeOtherBrackets) {
                    return getNext(closebrace, yytext()); }
                  else {
                    return getNext();
                  }
                }
\(		{ if (normalizeParentheses) {
                    return getNext(openparen, yytext()); }
                  else {
                    return getNext();
                  }
                }
\)		{ if (normalizeParentheses) {
                    return getNext(closeparen, yytext()); }
                  else {
                    return getNext();
                  }
                }
{HYPHENS}	{ if (yylength() >= 3 && yylength() <= 4 && ptb3Dashes) {
	            return getNext(ptbmdash, yytext());
                  } else {
                    return getNext();
		  }
		}
{LDOTS}		{ return handleEllipsis(yytext()); }
{FNMARKS}	{ return getNext(); }
{ASTS}		{ if (escapeForwardSlashAsterisk) {
                    return getNext(delimit(yytext(), '*'), yytext()); }
                  else {
                    return getNext();
                  }
                }
{INSENTP}	{ return getNext(); }
\.|\?|\!	{ return getNext(); }
=		{ return getNext(); }
\/		{ if (escapeForwardSlashAsterisk) {
                    return getNext(delimit(yytext(), '/'), yytext()); }
                  else {
                    return getNext();
                  }
                }
/* {HTHING}/[^a-zA-Z0-9.+]    { return getNext(); } */
{HTHING}        { return getNext(); }
{THING}		{ return getNext(); }
{THINGA}	{ if (normalizeAmpersandEntity) {
                    return getNext(normalizeAmp(yytext()), yytext()); }
                  else {
                    return getNext();
                  }
                }
'/[A-Za-z][^ \t\n\r\u00A0] { /* invert quote - often but not always right */
		  return handleQuotes(yytext(), true);
                }
{REDAUX}	{ return handleQuotes(yytext(), false); }
{QUOTES}	{ return handleQuotes(yytext(), false); }
\0|{SPACE}	{ if (invertible) {
                     prevWord.appendAfter(yytext());
                  } }
\r|\n|\r\n|\u2028|\u2029|\u000B|\u000C|\u0085	{
                  if (tokenizeNLs) {
                      return getNext(NEWLINE_TOKEN, yytext()); // js: for tokenizing carriage returns
                  } else if (invertible) {
                      prevWord.appendAfter(yytext());
                } }
&nbsp;		{ if (invertible) {
                     prevWord.appendAfter(yytext());
                  } }
.       { String str = yytext();
          if (invertible) {
            prevWord.appendAfter(str);
          }
          int first = str.charAt(0);
          String msg = String.format("Untokenizable: %s (char in decimal: %s)", yytext(), first);
          if (!this.seenUntokenizableCharacter) {
            LOGGER.warning(msg);
            this.seenUntokenizableCharacter = true;
          }
          LOGGER.fine(msg); }
<<EOF>> { if (invertible) { prevWord.appendAfter(yytext()); }
          return null; }
