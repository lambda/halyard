// -*- Mode: C++; tab-width: 4; -*-

#ifndef Typography_H
#define Typography_H

#include "TCommon.h"

#include <iostream>
#include <string>
#include <deque>
#include <list>
#include <map>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <wchar.h>

#include "TException.h"
#include "FileSystem.h"
#include "GraphicsTools.h"

// TODO - Handle copy constructors, assignment operators

namespace Typography {

	using GraphicsTools::Distance;
	using GraphicsTools::Point;
	using GraphicsTools::Image;
	using GraphicsTools::Color;

	//////////
	// A FreeType 2 vector, used for kerning.
   	typedef FT_Vector Vector;

	//////////
	// A FreeType 2 character code, used to represent a 32-bit
	// Unicode character.
	typedef FT_ULong CharCode;

	//////////
	// A FreeType 2 glyph index.  A (face,character code) pair map
	// to a glyph index in FreeType 2.
	typedef FT_UInt GlyphIndex;

	//////////
	// A FreeType 2 glyph object.  This usually contains a bitmap and
	// a whole bunch of measurements.
	typedef FT_GlyphSlot Glyph;

	//////////
	// Names for a few special Unicode characters.
	enum Character {

		//////////
		// This space is treated like a letter for the purposes of
		// linebreaking.
		// TODO - Fix line-breaking code to support it.
		kNonBreakingSpace	  = 0x00A0,

		//////////
		// A soft hyphen is treated as an invisible hyphenation point.
		// There's a lot of controversy about exactly what this means:
		//	 * ISO Latin 1 allegedly feels that it should always be printed
		//	   as a hyphen, but only allows it to appear at the end of a line.
		//	 * Unicode allows this character anywhere, but only allows it to
		//	   be visible when it appears before an implicitly inserted
		//	   line break chosen by the rendering engine.
		// We compromise: We allow it anywhere, and we use it as a line-
		// breaking hint.  But if it is the last non-whitespace character
		// before *any* kind of line-break, we display it.
		kSoftHyphen			  = 0x00AD,

		//////////
		// Unicode replacement character (typically drawn as a box).
		kReplacementCharacter = 0xFFFD,

		//////////
		// This is slightly magic in our library--we always map it
		// to a FreeType GlyphIndex of 0 (which is FreeType's "no glyph
		// for the given CharCode" GlyphIndex), and any other CharCode
		// kerned against it returns (0, 0).  Basically, it's
		// guaranteed to be a CharCode equivalent of FreeType's
		// GlyphIndex '0'.
		kNoSuchCharacter	  = kReplacementCharacter
	};

	//////////
	// Justification values for line layout.
	enum Justification {
		kLeftJustification,
		kCenterJustification,
		kRightJustification
	};

	//////////
	// A face style.  This is combined with a face, colors, and other
	// information to make a full-fledged Style.
	//
	enum FaceStyle {
		// These styles are directly supported by the FamilyDatabase.
		kRegularFaceStyle = 0,
		kBoldFaceStyle = 1,
		kItalicFaceStyle = 2,
		kBoldItalicFaceStyle = kBoldFaceStyle | kItalicFaceStyle,
		kIntrisicFaceStyles = kBoldFaceStyle | kItalicFaceStyle,

		// These styles are implemented manually.
		kUnderlineFaceStyle = 4,
		kShadowFaceStyle = 8
	};

	//////////
	// A Typography-related exception.  Any of the functions in the
	// Typography module may throw exceptions (which is not the case
	// for the rest of the 5L code base, so be sure to catch them).
	//
	class Error : public FIVEL_NS TException {
	public:
		explicit Error(int inErrorCode);
		explicit Error(const std::string &inErrorMessage)
			: TException(inErrorMessage) {}
		
		virtual const char *GetClassName() const
		    { return "Typography::Error"; }

		//////////
		// Check the result of a FreeType function and throw an error
		// if necessary.
		static void CheckResult(int inResultCode)
			{ if (inResultCode) throw Error(inResultCode); }
	};

	//////////
	// An instance of the FreeType 2 library's context.
	//
	class Library {
	private:
		FT_Library mLibrary;
		static Library *sLibrary;
		
	public:
		Library();
		~Library();

		operator FT_Library() { return mLibrary; }
		operator FT_Library*() { return &mLibrary; }

		static Library *GetLibrary();
	};

	class AbstractFace;
	class FaceStack;
	
	//////////
	// A style.
	//
	// This class must be quick to copy and efficient to store, so
	// we use an internal reference-counted 'rep' class.
	//
	class Style {
		struct StyleRep {
			int         mRefCount;

			std::string mFamily;
			std::list<std::string> mBackupFamilies;
			FaceStyle   mFaceStyle;
			int         mSize;
			Color       mColor;
			Color       mShadowColor;
			
			FaceStack   *mFace;
		};

		//////////
		// A pointer to our representation's data.
		//
		StyleRep *mRep;

		//////////
		// Make sure we don't share this representation with anybody else.
		//
		void Grab();

		//////////
		// Invalidate the cached mFace value.
		//
		void InvalidateFace();

	public:
		Style(const std::string &inFamily, int inSize);
		Style(const Style &inStyle);
		~Style();

		Style &operator=(const Style &inStyle);

		//////////
		// Get the font family.  e.g., "Times", "Nimbus Roman No9 L".
		//
		std::string GetFamily() const { return mRep->mFamily; }
		Style &SetFamily(const std::string &inFamily);

		//////////
		// Get other families to use when performing font substitution.
		//
		std::list<std::string> GetBackupFamilies() const
		    { return mRep->mBackupFamilies; }
		Style &SetBackupFamilies(const std::list<std::string> &inBFs);

		//////////
		// Get the style flags for this face.
		//
		FaceStyle GetFaceStyle() const { return mRep->mFaceStyle; }
		Style &SetFaceStyle(FaceStyle inFaceStyle);

		//////////
		// Get the size of the font, in points.
		//
		int GetSize() const { return mRep->mSize; }
		Style &SetSize(int inSize);

		//////////
		// Get the color used to draw text.
		//
		Style &SetColor(Color inColor);
		Color GetColor() const { return mRep->mColor; }

		//////////
		// Get the color used to draw drop-shadows.
		//
		Style &SetShadowColor(Color inColor);
		Color GetShadowColor() const { return mRep->mShadowColor; }

		// Drawing-related information.
		// TODO - Document.
		AbstractFace *GetFace() const;
		bool          GetIsUnderlined() const;
		bool          GetIsShadowed() const;
	};

	//////////
	// Style information for a piece of text.
	//
	class StyleInformation {

		//////////
		// An individual style run (internal).
		//
		struct StyleRun {
			int   offset;
			Style style;
			StyleRun(int inOffset, const Style &inStyle)
				: offset(inOffset), style(inStyle) {}
		};
		typedef std::list<StyleRun> StyleRunList;

		StyleRunList mStyleRuns;
		bool         mIsBuilt;
		int          mEnd;

	public:
		//////////
		// Create a new StyleInformation object.
		//
		// [in] inBaseStyle - The style to use for the first character.
		//
		explicit StyleInformation(const Style &inBaseStyle);

		//////////
		// Change the Style at the specified offset in the string.
		//
		// [in] inOffset - The offset at which to change the style.
		//                 This must be greater than or equal to the
        //                 inOffset argument to all previous calls to
        //                 ChangeStyleAt.
		// [in] inStyle -  The new style to use.
		//
		void ChangeStyleAt(int inOffset, const Style &inStyle);

		//////////
		// Stop adding style changes, and freeze the StyleInformation.
		//
		// [in] inOffset - The offset at which to end style information.
		//                 This typically corresponds to the end of the
		//                 string.  This must be greater than or equal
		//                 to all previous inOffset arguments.
		//
		void EndStyleAt(int inOffset);

		//////////
		// An STL-like iterator class for iterating over styles.
		// This class returns the Style objects for each offset, counting
		// by ones from zero (i.e., it returns 3 identical values for
		// a three-element style run).
		//
		class const_iterator {
			friend class StyleInformation;

			const StyleInformation *mStyleInfo;
			int mCurrentPosition;
			int mEndOfRun;
			StyleRunList::const_iterator mCurrentStyle;

			const_iterator(const StyleInformation *inStyleInfo, bool isEnd);
			
			void UpdateEndOfRun();
			void LoadNextStyleRun();

		public:
			//////////
			// Construct an empty iterator.  Don't use this for anything
			// until you assign a real iterator to it.
			//
			const_iterator();

			const_iterator &operator++()
			{
				ASSERT(mStyleInfo != NULL);
				ASSERT(mCurrentPosition < mStyleInfo->mEnd);
				++mCurrentPosition;
				if (mCurrentPosition == mEndOfRun)
					LoadNextStyleRun();
				return *this;
			}

			const_iterator &operator--();

		    bool operator==(const const_iterator &inRight) const
			{
				ASSERT(mStyleInfo != NULL);
				ASSERT(mStyleInfo == inRight.mStyleInfo);
				return mCurrentPosition == inRight.mCurrentPosition;
			}

			bool operator!=(const const_iterator &inRight) const
			{
				ASSERT(mStyleInfo != NULL);
				ASSERT(mStyleInfo == inRight.mStyleInfo);
				return mCurrentPosition != inRight.mCurrentPosition;
			}

			const Style &operator*() const
			{
				ASSERT(mStyleInfo != NULL);
				ASSERT(mCurrentPosition < mStyleInfo->mEnd);
				return mCurrentStyle->style;
			}

			const Style *operator->() const
			{
				return &operator*();
			}
		};
		
		friend class const_iterator;

		//////////
	    // Return an iterator pointing to the first element's style.
		//
		const_iterator begin() const { return const_iterator(this, false); }

		//////////
	    // Return an iterator pointing one past the last element's style.
		// Do not dereference this.
		//
		const_iterator end() const { return const_iterator(this, true); }
	};

	class Face;

	//////////
	// An abstract typeface (with a specific size).
	//
	// Abstract typefaces know how to map character codes to glyphs,
	// kern two characters, and calculate the height of a line.
	// They don't know about GlyphIndex values or other low-level
	// abstractions.
	//
	class AbstractFace {
		int mSize;
		
	public:
		explicit AbstractFace(int inSize) : mSize(inSize) { }
		virtual ~AbstractFace() { }

		int GetSize() const { return mSize; }

		//////////
		// Load the glyph for a given character code.
		// You're free to use this pointer until the next call
		// to GetGlyph.  The face object retains ownership of the
		// pointer, so you shouldn't delete it.  If no glyph exists
		// for the specified character code, the face will return
		// a substitution character.
		//
		// For now, the glyph is always rendered to a pixmap.  This
		// may or may not change.
		//
		virtual Glyph GetGlyph(CharCode inCharCode) = 0;

		//////////
		// Return a best guess for the appropriate distance between
		// two lines.  This relies on the font's data tables and
		// FreeType's drivers, so it might occasionally be a bit whacky.
		// (But it's all we've got.)
		//
		virtual Distance GetLineHeight() = 0;

		//////////
		// Find the concrete face object which would be used to draw the
		// specified character.
		//
		// [in] inCharCode - The character whose face we want to find.
		// [out] return -    The face associated with that character.
		//                   (This is returned as a pointer because
		//                   Face can't be declared until AbstractFace
		//                   has been declared.  This pointer points
		//                   to somebody else's face object; don't
		//                   destroy it.)
		//
		virtual Face *GetRealFace(CharCode inCharCode) = 0;

		//////////
		// Kern two characters.  If either character code is
		// kNoSuchCharacter, or either face is NULL, this function will
		// return (0,0).
		//
		// [in] inChar1 - The first character, or kNoSuchCharacter.
		// [in] inFace1 - The face of the first character, or NULL.
		// [in] inChar2 - The second character, or kNoSuchCharacter.
		// [in] inFace2 - The face of the second character, or NULL.
		// [out] result - The amount to kern.
		//
		static Vector Kern(CharCode inChar1, AbstractFace *inFace1,
						   CharCode inChar2, AbstractFace *inFace2);
	};

	//////////
	// A simple typeface.
	//
	// A simple typeface is associated with a single FreeType 2 face
	// object.  It understands GlyphIndex values and other low-level
	// details of layout.
	//
	class Face : public AbstractFace {
		// Refcounting so we can implement copy & assignment.  The use of
		// internal "rep" objects is a common C++ technique to avoid
		// copying heavyweight data structures around.  In our case, we
		// simply *can't* copy the underlying FreeType data.
		struct FaceRep {
			FT_Face mFace;
			int mRefcount;

			FaceRep(FT_Face inFace) : mFace(inFace), mRefcount(1) {}
			~FaceRep() { Error::CheckResult(FT_Done_Face(mFace)); }
		};

		FaceRep *mFaceRep;
		bool mHasKerning;

	public:
		Face(const char *inFontFile, const char *inMetricsFile,
			 int inSize);
		Face(const Face &inFace);
		virtual ~Face();
		
		operator FT_Face() { return mFaceRep->mFace; }
		operator FT_Face*() { return &mFaceRep->mFace; }

		std::string GetFamilyName () const
		    { return std::string(mFaceRep->mFace->family_name); }
		std::string GetStyleName () const
		    { return std::string(mFaceRep->mFace->style_name); }

		GlyphIndex GetGlyphIndex(CharCode inCharCode);
		Glyph GetGlyphFromGlyphIndex(GlyphIndex inGlyphIndex);

		Glyph GetGlyph(CharCode inCharCode);

		//////////
		// Kern two character codes.  If either character code
		// is kNoSuchCharacter, this function will return (0,0).
		//
		Vector GetKerning(CharCode inPreviousChar, CharCode inCurrentChar);

		Distance GetLineHeight();

		Face *GetRealFace(CharCode inCharCode) { return this; }

		//////////
		// Return true if and only if two 'Face' objects have the same
		// underlying FT_Face.
		//
		friend bool operator==(const Face &inLeft, const Face &inRight)
		    { return inLeft.mFaceRep->mFace == inRight.mFaceRep->mFace; }

		//////////
		// Return true if and only if two 'Face' objects don't have the
		// same underlying FT_Face.
		//
		friend bool operator!=(const Face &inLeft, const Face &inRight)
		    { return !(inLeft == inRight); }
	};

	//////////
	// A "stack" of simple typefaces.
	//
	// A FaceStack is used to provide more complete character sets than any
	// single typeface could provide alone.  We search through the faces
	// in each stack, beginning with the "primary" face, and then each
	// of the "secondary" faces.
	//
	// A typical use: Font FooSerif is a nice text face, but lacks a
	// delta character.  We can create a stack using FooSerif as a primary
	// face and Symbol as a second face, and the engine will do
	// The Right Thing<tm>.
	//
	// All faces in a a stack must be the same size.
	//
	class FaceStack : public AbstractFace {
		std::deque<Face> mFaceStack;

	public:
		explicit FaceStack(const Face &inPrimaryFace);
		virtual ~FaceStack();

		//////////
		// Add another face to the stack.  Faces are searched
		// in the order they are added.
		//
		void AddSecondaryFace(const Face &inFace);

		virtual Glyph GetGlyph(CharCode inCharCode);
		virtual Distance GetLineHeight();
		virtual Face *GetRealFace(CharCode inCharCode);

	private:
		//////////
		// Walk though the stack, looking a face with an appropriate
		// glyph for the specified character.
		//
		// [in] inCharCode - The character we want to find.
		// [out] outFace - A face with an appropriate glyph.
		// [out] outGlyphIndex - The index of the glyph in that face.
		void SearchForCharacter(CharCode inCharCode,
								Face **outFace,
								GlyphIndex *outGlyphIndex);
	};


	//////////
	// A representation of a styled character.  The 'style' member
	// is really a reference to a style stored elsewhere (for performance).
	//
	struct StyledChar {
		const wchar_t value;
		const Style &style;

		StyledChar(wchar_t inValue, const Style &inStyle)
			: value(inValue), style(inStyle) {}
	};

	//////////
	// An iterator over a chunk of styled text.
	//
	// This is a wrapper around the iterators for characters and styles.
	// By creating a combined interface, we make our code look a lot
	// nicer.
	//
	class StyledCharIterator {
		friend struct StyledTextSpan;

		const wchar_t *mTextIter;
	    StyleInformation::const_iterator mStyleIter;

	public:
		StyledCharIterator(const wchar_t *inTextIter,
						   const StyleInformation::const_iterator &inStyleIter)
			: mTextIter(inTextIter), mStyleIter(inStyleIter) {}

		StyledCharIterator() : mTextIter(NULL) {}
		
		StyledCharIterator &operator++()
		    { ++mTextIter; ++mStyleIter; return *this; }
		StyledCharIterator &operator--()
		    { --mTextIter; --mStyleIter; return *this; }
		bool operator==(const StyledCharIterator &inRight) const
		    { return mTextIter == inRight.mTextIter; }
		bool operator!=(const StyledCharIterator &inRight) const
		    { return mTextIter != inRight.mTextIter; }
		const StyledChar operator*() const
		    { return StyledChar(*mTextIter, *mStyleIter); }
	};

	//////////
	// A chunk of styled text.  This object contains no actual
	// data; it merely points to data stored elsewhere.  It's a bit
	// of a strange abstraction--but it was the natural result of
	// refactoring.
	//
	// Make sure you don't deallocate the underlying string or
	// style information while this object exists.
	// 
	struct StyledTextSpan {

		//////////
		// A pointer to the first character in the span.
		//
		StyledCharIterator begin;

		//////////
		// A pointer one character *beyond* the last character in
		// the span, as per STL iterator conventions.
		//
		StyledCharIterator end;

		StyledTextSpan(const StyledCharIterator &inBegin,
					   const StyledCharIterator &inEnd);
		StyledTextSpan() {}
	};

	//////////
	// A segment of a line of characters, suitable for drawing as a group.
	//
	// The line-breaking routines all work in terms of line segments
	// (because they don't want to know about the rules for breaking up
	// a line into words).
	//
	// Some interesting invariants:
	//   * A line segment is always non-empty.
	//   * A line segment will never contain a soft hyphen.
	// If you change these invariants, be prepared to face assertion
	// failures and debugging fun.
	//
	// TODO - Turn this struct into a well-encapsulated class; it's
	// becoming too complex to be a struct.
	//
	struct LineSegment {

		//////////
		// The text span included in this segment.
		//
		StyledTextSpan span;

		//////////
		// Is the current segment a newline character?  If so, the
		// segment contains no displayable data.
		//
		bool isLineBreak;

		//////////
		// Should the segment be discarded at the end of a line?
		// This is typically true for whitespace.
		// TODO - This is used as a 'isHorizontalWhitespace' flag,
		// so we should probably rename it.
		//
		bool discardAtEndOfLine;

		//////////
		// If this segment is the last on a line, do we need to draw a
		// hyphen?  Typically true for segments preceding a soft hyphen,
		// or segments which were automatically broken by the library.
		//
		bool needsHyphenAtEndOfLine;

		// Used by various algorithms to temporarily store data.
		// XXX - Clean up.
		Distance userDistanceData;

		void SetLineSegment(const StyledTextSpan &inSpan,
							bool inIsLineBreak = false,
							bool inDiscardAtEndOfLine = false,
							bool inNeedsHyphenAtEndOfLine = false)
		{
			span				   = inSpan;
			isLineBreak			   = inIsLineBreak;
			discardAtEndOfLine	   = inDiscardAtEndOfLine;
			needsHyphenAtEndOfLine = inNeedsHyphenAtEndOfLine;
			userDistanceData	   = 0;
		}

		explicit LineSegment(const StyledTextSpan &inSpan,
							 bool inIsLineBreak = false,
							 bool inDiscardAtEndOfLine = false,
							 bool inNeedsHyphenAtEndOfLine = false);

		LineSegment() {}
	};

	extern bool operator==(const LineSegment &left, const LineSegment &right);

	//////////
	// An iterator which will break a line into LineSegment objects
	// according to typical typographic rules.  It does not modify the
	// underlying text.
	class LineSegmentIterator {
		StyledCharIterator mSegmentBegin;
		StyledCharIterator mTextEnd;

	public:
		//////////
		// Create a new iterator.
		//
		// [in] inSpan - The text to break into segments.
		//
		explicit LineSegmentIterator(const StyledTextSpan &inSpan);

		//////////
		// Return the next segment of the line, if any.
		//
		// [out] outSegment - The segment we found, or unchanged.
		// [out] return - True iff we found another segment.
		//
		bool NextElement(LineSegment *outSegment);
	};

	//////////
	// Display-independent code to transform text into a multi-line
	// paragraph.
	//
	// GenericTextRenderingEngine is an abstract class; subclasses
	// must provide support for measuring LineSegment objects,
	// forcibly breaking segments longer than a line, and drawing
	// all the segments on a line.
	//
	// The 'Distance' values used by this class are abstract--they
	// might be pixels, or they might be simple character counts.
	// The GenericTextRenderingEngine doesn't care.  (You could use
	// it to line-break and justify character strings, and the
	// test suites actually do so.)
	//
	class GenericTextRenderingEngine {
	private:
		LineSegmentIterator mIterator;
		Distance mLineLength;
		Justification mJustification;

	protected:
		//////////
		// Create a new GenericTextRenderingEngine.
		// 
		// [in] inSpan - The text to draw.
		// [in] inLineLength - Maximum allowable line length.
		// [in] inJustification - Justification for the line.
		//
		GenericTextRenderingEngine(const StyledTextSpan &inSpan,
								   Distance inLineLength,
								   Justification inJustification);

		virtual ~GenericTextRenderingEngine() {}
		
		Distance GetLineLength() { return mLineLength; }
		Justification GetJustification() { return mJustification; }

		//////////
		// Subclasses must override this method to provide measurements
		// of segments.  If there is a previous segment, it will be
		// supplied so intersegment kerning may be calculated (if the
		// subclass so desires).  Measurements for a given segment are
		// allowed to differ, depending on whether or not the segment
		// appears at the end of a line.  (This is useful for handling
		// the LineSegment::needsHyphenAtEndOfLine field.)
		// 
		// [in] inPrevious -    The previous segment (for kerning), or NULL.
		// [in] inSegment -     The segment to measure.
		// [in] inAtEndOfLine - Should measurements assume this is the last
		//                      segment on the line?
		//
		virtual Distance MeasureSegment(LineSegment *inPrevious,
										LineSegment *inSegment,
										bool inAtEndOfLine) = 0;

		//////////
		// Subclasses must override this method to forcibly extract a
		// line's worth of data from the front of a segment.  Subclasses
		// are welcome to throw exceptions if this process fails.
		// 
		// [in/out] ioRemaining - On input, the segment from which to
		//                        extract a line.  On output, whatever
		//                        is left over after extraction.  (Remember,
		//                        segments must be non-empty!)
		// [out] outExtracted -   A segment that fits on one line.
		//
		virtual void ExtractOneLine(LineSegment *ioRemaining,
									LineSegment *outExtracted) = 0;

		//////////
		// Subclasses must override this method to actually display a
		// line.
		//
		// [in] inLine - A list of segments to display.
		// [in] inHorizontalOffset - The distance to indent this line.
		//
		virtual void RenderLine(std::deque<LineSegment> *inLine,
								Distance inHorizontalOffset) = 0;

	public:
		//////////
		// Actually draw the text.  This method may only be called
		// once.  (Should we enforce and/or fix this?)
		void RenderText();

	private:
		//////////
		// Given the space used by a line, calculate the appropriate
		// amount to indent the line to acheive the desired justification.
		Distance CalculateHorizontalOffset(Distance inSpaceUsed);

		//////////
		// Internal routine which calculates justification, calls
		// RenderLine, and removes all the segments from ioLine.
		void RenderAndResetLine(std::deque<LineSegment> *ioLine);
	};

	//////////
	// A real rendering engine which uses real fonts.
	//
	// We subclass GenericTextRenderingEngine, and provide support for
	// AbstractFaces, drawing positions, and output to images.
	//
	// We assume that all Distance and Point values are measured in pixels.
	// 
	class TextRenderingEngine : public GenericTextRenderingEngine {
		Image *mImage;
		Point mLineStart;

	public:
		//////////
		// Create a new text rendering engine.
		//
		// [in] inSpan -       The styled text to draw.
		// [in] inPosition -   The x,y position of the lower-left corner
		//                     of the first character (actually, this
		//                     is technically the "origin" of the first
		//                     character in FreeType 2 terminology).
		// [in] inLineLength - The maximum number of pixels available for
		//                     a line.  This is (I hope) a hard limit,
		//                     and no pixels should ever be drawn beyond it.
		// [in] inJustification - The desired justification.
		// [in] inImage -      The image into which we should draw.
		//                     This must not be deallocated until the
		//                     TextRendering engine is destroyed.
		//
		TextRenderingEngine(const StyledTextSpan &inSpan,
							Point inPosition,
							Distance inLineLength,
							Justification inJustification,
							Image *inImage);

	private:
		//////////
		// Draw a FreeType 2 bitmap to our image.
		//
		// [in] inBitmap -   The bitmap to draw (in FreeType 2 format).
		// [in] inPosition - The location at which to draw the bitmap.
		//
		void DrawBitmap(Point inPosition, FT_Bitmap *inBitmap, Color inColor);

	protected:
		virtual Distance MeasureSegment(LineSegment *inPrevious,
										LineSegment *inSegment,
										bool inAtEndOfLine);

		virtual void ExtractOneLine(LineSegment *ioRemaining,
									LineSegment *outExtracted);

		virtual void RenderLine(std::deque<LineSegment> *inLine,
			                    Distance inHorizontalOffset);
    };

	//////////
	// A FamilyDatabase knows how to load fonts from disk, given
	// a family name, a FaceStyle and a size in points.
	// For example: ("Times", kBoldItalicFaceStyle, 12).
	//
	// A FamilyDatabase will create and use an on-disk font cache to avoid
	// opening zillions of font files at application startup.
	//
	// <h2>FamilyDatabase Internals</h2>
	//
	// There are four levels of organization within a family database.
	// From lowest to highest, these are:
	//
	//   * AvailableFace - Stores information about a single font file.
	//   * FaceSizeGroup - Stores information about all the font files
	//     with the same family name and FaceStyle.
	//   * Family - Stores information about all the fonts files with
	//     the same family name.
	//   * FamilyDatabase - Stores information about all available fonts.
	//
	// AvailableFaces move downward from FamilyDatabase objects to
	// FaceSizeGroup objects using a series of AddAvailableFace methods.
	// Face objects move upward from FaceSizeGroup objects to
	// FamilyDatabase objects using a series of GetFace methods.
	//
	class FamilyDatabase {
	public:
		//////////
		// We represent scalable faces by a size of kAnySize.  This is only
		// public because MSVC++ won't allow nested classes to access
		// private members; you can't do anything with it.
		//
		enum { kAnySize = 0 };
		
	private:
		//////////
		// An AvailableFace stores information about a single face on disk.
		// This is the lowest level of data storage in the FamilyDatabase.
		//
		class AvailableFace {
			std::string mFileName;
			
			int         mSize;
			std::string mFamilyName;
			std::string mStyleName;
			bool        mIsBold;
			bool        mIsItalic;
			
		public:
			//////////
			// Create a new AvailableFace, loading various information from
			// disk.  You must specify 'inFileName' relative to the 'Font'
			// directory (this is so we don't have to portably serialize
			// FileSystem::Path objects to the cache, which would by icky).
			//
			explicit AvailableFace(const std::string &inFileName);
			
			int         GetSize() const { return mSize; }
			std::string GetFamilyName() const { return mFamilyName; }
			std::string GetStyleName() const { return mStyleName; }
			bool        IsBold() const { return mIsBold; }
			bool        IsItalic() const { return mIsItalic; }
			bool        IsScalable() const { return GetSize() == kAnySize; }
			
			//////////
			// Load this face as a 'Face' object, using the specified
			// size.  If the font is not available in this size, the
			// behavior of this function is undefined.
			//
			Face   OpenFace(int inSize) const;
			
			//////////
			// Write some face cache header information to an ostream.
			//
			static void WriteSerializationHeader(std::ostream &out);

			//////////
			// Read the face cache header information from an ostream,
			// and validate it.
			//
			static void ReadSerializationHeader(std::istream &in);
		
			//////////
			// Construct an AvailableFace object using cache data from
			// a stream, and advance to the start of the next face object.
			//
			AvailableFace(std::istream &in);

			//////////
			// Serialize an AvailableFace object to a stream as cache data.
			//
			void        Serialize(std::ostream &out) const;
		};
		
		//////////
		// A FaceSizeGroup stores all the AvailableFace objects for a
		// given (family name, face style) pair.  It is the second-lowest
		// level of organization in a FamilyDatabase, after AvailableFace.
		//
		// It may contain a single scalable face (say, "Nimbus Mono",
		// kRegularFaceStyle), a set of bitmap fonts in various sizes (all
		// the italic "Times" faces), or some combination of the above.
		//
		// A FaceSizeGroup keeps a cache of all Face objects it opens
		// on behalf of the user.
		//
		class FaceSizeGroup {
			std::map<int,AvailableFace> mAvailableFaces;
			std::map<int,Face> mFaces;
			
		public:
			FaceSizeGroup() {}
			
			void AddAvailableFace(const AvailableFace &inFace);
			Face GetFace(int inSize);
			
			void Serialize(std::ostream &out) const;
		};
		
		//////////
		// A Family stores all the various sizes and styles for a given
		// font family (e.g., "Times", "Nimbus Mono").  It's the
		// third-lowest level of organization in a FamilyDatabase.
		//
		// If bold or italic faces are missing, a Family object will
		// try to find an appropriate substitute in a different style.
		// 
		class Family {
			std::string   mFamilyName;
			
			FaceSizeGroup mRegularFaces;
			FaceSizeGroup mBoldFaces;
			FaceSizeGroup mItalicFaces;
			FaceSizeGroup mBoldItalicFaces;
			
		public:
			explicit Family(const std::string &inFamilyName)
				: mFamilyName(inFamilyName) {}
			
			void AddAvailableFace(const AvailableFace &inFace);
			Face GetFace(FaceStyle inStyle, int inSize);
			
			void Serialize(std::ostream &out) const;
		};
		
	private:
		std::map<std::string,Family> mFamilyMap;

		//////////
		// Does the file pointed to by 'inPath' look like a font file?
		//
		static bool IsFontFile(const FileSystem::Path &inPath);
		
		//////////
		// Store an AvailableFace object in an appropriate place
		// in the database.
		//
		void AddAvailableFace(const AvailableFace &inFace);

		static FamilyDatabase *sFamilyDatabase;
		
	public:
		//////////
		// Get a global instance of FamilyDatabase.
		//
		static FamilyDatabase *GetFamilyDatabase();

		FamilyDatabase() {}
		
		//////////
		// Load all the fonts in the application's Font directory.
		//
		void ReadFromFontDirectory();

		//////////
		// Read in available font information from a font cache.
		//
		void ReadFromCache(std::istream &in);

		//////////
		// Write the entire database to a font cache.
		//
		void WriteToCache(std::ostream &out) const;
		
		//////////
		// Look up a face.
		//
		// [in] inFamilyName - The family (e.g., "Nimbus Mono").
		// [in] inStyle -      The style.  May be kRegularFaceStyle,
		//                     kBoldFaceStyle, kItalicFaceStyle,
		//                     or kBoldItalicFaceStyle.  Other styles
		//                     are generated by the drawing routines.
		// [in] inSize -       A font size, in points.
		// [out] return -      An appropriate Face object.
		//
		Face GetFace(const std::string &inFamilyName,
					 FaceStyle inStyle, int inSize);
	};
}

#endif // Typography_H
