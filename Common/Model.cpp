// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; -*-

#include <boost/lexical_cast.hpp>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "TCommon.h"
#include "Model.h"
#include "ModelChange.h"

USING_NAMESPACE_FIVEL
using namespace model;
using namespace model::Private;
using boost::lexical_cast;


//=========================================================================
//  Private Data Types
//=========================================================================

DatumMap::~DatumMap()
{
	for (iterator i = begin(); i != end(); i++)
		delete i->second;
}

DatumVector::~DatumVector()
{
	for (iterator i = begin(); i != end(); i++)
		delete *i;	
}




//=========================================================================
//  Class Methods
//=========================================================================

Class::ClassMap *Class::sClasses = NULL;

Class::Class(const std::string &inName, CreatorFunction inCreator)
	: mName(inName), mCreator(inCreator)
{
	ASSERT(mName != "");
	ASSERT(mCreator != NULL);
	
	// We have to allocate this map dynamically, because we're called
	// for static member variables, and our own static constructor
	// may not have been called yet.
	if (!sClasses)
		sClasses = new ClassMap();

	ClassMap::iterator found = sClasses->find(mName);
	if (found != sClasses->end())
		THROW("Attempted to register a model::Class twice");
	sClasses->insert(ClassMap::value_type(mName, this));
}

Object *Class::CreateInstance() const
{
	return (*mCreator)();
}

Class *Class::FindByName(const std::string &inClass)
{
	ASSERT(sClasses);
	ClassMap::iterator found = sClasses->find(inClass);
	if (found == sClasses->end())
		THROW("Attempted to create unknown model::Class");
	return found->second;
}


//=========================================================================
//  Datum Methods
//=========================================================================

Datum *Datum::CreateFromXML(xml_node inNode)
{
	std::string type  = inNode.attribute("type");
	if (type == "int")
		return new Integer(boost::lexical_cast<int>(inNode.text()));
	else if (type == "str")
		return new String(inNode.text());
	else if (type == "map")
		return new Map();
	else if (type == "list")
		return new List();
 	else if (type == "object")
		return Class::FindByName(inNode.attribute("class"))->CreateInstance();
	else
		THROW("Unsupported XML data type");
}


//=========================================================================
//  ValueDatum Implementation
//=========================================================================

void Integer::Write(xml_node inContainer)
{
	inContainer.set_attribute("type", "int");
	inContainer.append_text(boost::lexical_cast<std::string>(mValue));
}

void String::Write(xml_node inContainer)
{
	inContainer.set_attribute("type", "str");
	inContainer.append_text(mValue);
}


//=========================================================================
//  MutableDatum Methods
//=========================================================================

void MutableDatum::ApplyChange(Change *inChange)
{
	ASSERT(mModel != NULL);
	ASSERT(inChange != NULL);
	mModel->ApplyChange(inChange);
}

void MutableDatum::RegisterChildWithModel(Datum *inDatum)
{
	ASSERT(mModel != NULL);
	ASSERT(inDatum != NULL);
	inDatum->RegisterWithModel(mModel);
}

void MutableDatum::RegisterWithModel(Model *inModel)
{
	ASSERT(mModel == NULL);
	ASSERT(inModel != NULL);
	mModel = inModel;
}


//=========================================================================
//  CollectionDatum Methods
//=========================================================================
//  Supported types of changes
//    Map container
//      Set
//        mContainer
//        mKey
//        mOldDatum (optional) *
//        mNewDatum
//        find(key) -> datum-or-null
//        remove-known-datum(key, datum)
//        insert(key, datum)
//      Delete
//        mContainer
//        mKey
//        mOldDatum *
//        find(key) -> datum-or-null
//        remove-known-datum(key, datum)
//        insert(key, datum)
//    List container
//      Set
//        mContainer
//        mKey
//        mOldDatum (optional) *
//        mNewDatum
//        find(key) -> datum-or-null
//        remove-known-datum(key, datum)
//        insert(key, datum)
//      Insert
//        mContainer
//        mKey
//        mNewDatum
//        insert(key, datum)
//        remove-known-datum(key, datum)
//      Delete
//        mContainer
//        mKey
//        mOldDatum *
//        find(key) -> datum-or-null
//        remove-known-datum(key, datum)
//        insert(key, datum)
//    Multiple containers
//      Transfer
//        mOldContainer
//        mOldKey
//        mNewContainer
//        mNewKey
//        mOldDatum (optional) *
//        mNewDatum
//        find(key) -> datum-or-null
//        remove-known-datum(key, datum)
//        insert(key, datum)


//=========================================================================
//  CollectionDatum Methods
//=========================================================================

template <typename KeyType>
void CollectionDatum<KeyType>::PerformSet(ConstKeyType &inKey, Datum *inValue)
{
	RegisterChildWithModel(inValue);
	ApplyChange(new SetChange<KeyType>(this, inKey, inValue));
}

template <typename KeyType>
void CollectionDatum<KeyType>::PerformDelete(ConstKeyType &inKey)
{
	ApplyChange(new DeleteChange<KeyType>(this, inKey));
}

template class model::CollectionDatum<std::string>;
template class model::CollectionDatum<size_t>;


//=========================================================================
//  HashDatum Methods
//=========================================================================

Datum *HashDatum::DoGet(ConstKeyType &inKey)
{
	DatumMap::iterator found = mMap.find(inKey);
	CHECK(found != mMap.end(), "Map::Get: Can't find key");
	return found->second;
}

Datum *HashDatum::DoFind(ConstKeyType &inKey)
{
	DatumMap::iterator found = mMap.find(inKey);
	if (found == mMap.end())
		return NULL;
	return found->second;
}

void HashDatum::DoRemoveKnown(ConstKeyType &inKey, Datum *inDatum)
{
	DatumMap::iterator found = mMap.find(inKey);
	ASSERT(found != mMap.end());
	ASSERT(found->second == inDatum);
	mMap.erase(found);
}

void HashDatum::DoInsert(ConstKeyType &inKey, Datum *inDatum)
{
	mMap.insert(DatumMap::value_type(inKey, inDatum));
}


//=========================================================================
//  Map Methods
//=========================================================================

void Map::Write(xml_node inContainer)
{
	inContainer.set_attribute("type", "map");
	DatumMap::iterator i = begin();
	for (; i != end(); ++i)
	{
		xml_node item = inContainer.new_child("item");
		item.set_attribute("key", i->first);
		i->second->Write(item);
	}
}

void Map::Fill(xml_node inNode)
{
	xml_node::iterator i = inNode.begin();
	for (; i != inNode.end(); ++i)
	{
		xml_node node = *i;
		XML_CHECK_NAME(node, "item");
		std::string key = node.attribute("key");
		Datum *value = CreateFromXML(node);
		Set(key, value);
		value->Fill(node);
	}
}


//=========================================================================
//  Object Methods
//=========================================================================

void Object::Write(xml_node inContainer)
{
	inContainer.set_attribute("type", "object");
	inContainer.set_attribute("class", mClass->GetName());
	DatumMap::iterator i = begin();
	for (; i != end(); ++i)
	{
		xml_node item = inContainer.new_child(i->first.c_str());
		i->second->Write(item);
	}
}

void Object::Fill(xml_node inNode)
{
	xml_node::iterator i = inNode.begin();
	for (; i != inNode.end(); ++i)
	{
		xml_node node = *i;
		std::string key = node.name();
		Datum *value = CreateFromXML(node);
		Set(key, value);
		value->Fill(node);
	}
}


//=========================================================================
//  List Methods
//=========================================================================

void List::PerformInsert(ConstKeyType &inKey, Datum *inValue)
{
	RegisterChildWithModel(inValue);
	ApplyChange(new InsertChange(this, inKey, inValue));
}

void List::Write(xml_node inContainer)
{
	inContainer.set_attribute("type", "list");
	DatumVector::iterator i = mVector.begin();
	for (; i != mVector.end(); ++i)
	{
		xml_node node = inContainer.new_child("item");
		(*i)->Write(node);
	}
}

void List::Fill(xml_node inNode)
{
	xml_node::iterator i = inNode.begin();
	for (; i != inNode.end(); ++i)
	{
		xml_node node = *i;
		XML_CHECK_NAME(node, "item");
		Datum *value = CreateFromXML(node);
		Insert(mVector.size(), value);
		value->Fill(node);
	}
}

Datum *List::DoGet(ConstKeyType &inKey)
{
	CHECK(inKey < mVector.size(), "No such key in List::DoGet");
	return mVector[inKey];
}

Datum *List::DoFind(ConstKeyType &inKey)
{
	if (inKey < mVector.size())
		return mVector[inKey];
	else
		return NULL;
}

void List::DoRemoveKnown(ConstKeyType &inKey, Datum *inDatum)
{
	// This runs in O(N) time, which isn't great.
	ASSERT(inKey < mVector.size());
	ASSERT(inDatum != NULL);
	ASSERT(mVector[inKey] == inDatum);
	mVector.erase(mVector.begin() + inKey);
}

void List::DoInsert(ConstKeyType &inKey, Datum *inDatum)
{
	// This runs in O(N) time, which isn't great.
	ASSERT(inKey <= mVector.size());
	ASSERT(inDatum != NULL);
	mVector.insert(mVector.begin() + inKey, inDatum);
}


//=========================================================================
//  MoveChange
//=========================================================================

#if 0
template <typename DestKeyType, typename SrcKeyType>
class MoveChange : public Change {
	typedef CollectionDatum<DestKeyType> DestCollection;
	typedef CollectionDatum<SrcKeyType> SrcCollection;
	typedef DestCollection::ConstKeyType DestConstKeyType;
	typedef SrcCollection::ConstKeyType SrcConstKeyType;

	SrcCollection *mOldCollection;
	SrcConstKeyType mOldKey;
	DestCollection *mNewCollection;
	DestConstKeyType mNewKey;
	Datum *mReplacedDatum;
	Datum *mMovedDatum;

public:
	MoveChange(DestCollection *inDest, DestConstKeyType &inDestKey,
			   SrcCollection *inSrc, SrcConstKeyType &inSrcKey);

protected:
	virtual void DoApply();
	virtual void DoRevert();
	virtual void DoFreeApplyResources();
	virtual void DoFreeRevertResources();
};

template <typename DestKeyType, typename SrcKeyType>
MoveChange<DestKeyType,SrcKeyType>::MoveChange(DestCollection *inDest,
											   DestConstKeyType &inDestKey,
											   SrcCollection *inSrc,
											   SrcConstKeyType &inSrcKey)
	: mOldCollection(inSrc), mOldKey(inSrcKey),
	  mNewCollection(inDest), mNewKey(inDestKey),
	  mReplacedDatum(NULL), mMovedDatum(NULL)
{
	mReplacedDatum = mNewCollection->DoFind(mNewKey);
	mMovedDatum = mOldCollection->DoFind(mOldKey);
	if (!mMovedDatum)
	{
		ConstructorFailing();
		THROW("DeleteChange: No such key");
	}
}

template <typename DestKeyType, typename SrcKeyType>
void MoveChange<DestKeyType,SrcKeyType>::DoApply()
{
	if (mReplacedDatum)
		mNewCollection->DoRemoveKnown(mNewKey, mReplacedDatum);
	mOldCollection->DoRemoveKnown(mOldKey, mMovedDatum);
	mNewCollection->DoInsert(mNewKey, mMovedDatum);
}

template <typename DestKeyType, typename SrcKeyType>
void MoveChange<DestKeyType,SrcKeyType>::DoRevert()
{
	mNewCollection->DoRemoveKnown(mNewKey, mMovedDatum);
	mOldCollection->DoInsert(mOldKey, mMovedDatum);
	if (mReplacedDatum)
		mNewCollection->DoInsert(mNewKey, mReplacedDatum);
}

template <typename DestKeyType, typename SrcKeyType>
void MoveChange<DestKeyType,SrcKeyType>::DoFreeApplyResources()
{
}

template <typename DestKeyType, typename SrcKeyType>
void MoveChange<DestKeyType,SrcKeyType>::DoFreeRevertResources()
{
	if (mReplacedDatum)
	{
		delete mReplacedDatum;
		mReplacedDatum = NULL;
	}
}

template <typename C1, typename C2>
void model::Move(C1 *inDest, typename C1::ConstKeyType &inDestKey,
					 C2 *inSrc, typename C2::ConstKeyType &inSrcKey)
{
	inDest->GetModel()->
      ApplyChange(new MoveChange<typename C1::KeyType,
				                 typename C2::KeyType>(inDest, inDestKey,
													   inSrc, inSrcKey));
}

template MoveChange<size_t,size_t>;
template MoveChange<std::string,size_t>;
template MoveChange<size_t,std::string>;
template MoveChange<std::string,std::string>;

template void model::Move(List*, List::ConstKeyType&,
						  List*, List::ConstKeyType&);

template void model::Move(Map*, Map::ConstKeyType&,
						  Map*, Map::ConstKeyType&);
#endif // 0


//=========================================================================
//  Model Methods
//=========================================================================

void Model::Initialize()
{
	mChangePosition = mChanges.begin();
	mRoot = std::auto_ptr<Map>(new Map());
	mRoot->RegisterWithModel(this);
}

Model::Model(const ModelFormat &inFormat)
	: mFormat(inFormat), mRoot(NULL)
{
	Initialize();
}

Model::Model(const ModelFormat &inCurrentFormat,
			 ModelFormat::Version inEarliestFormat,
			 const std::string &inPath)
	: mFormat(inCurrentFormat), mRoot(NULL)
{
	typedef ModelFormat::Version Version;

	Initialize();

	// Open the XML file.
	xmlDocPtr doc = xmlParseFile(inPath.c_str());
	try
	{
		CHECK(doc, "Failed to load XML file");

		// Get the root element.
		xmlNodePtr root_node = xmlDocGetRootElement(doc);
		CHECK(root_node, "No document root in XML file");
		xml_node root(root_node);

		// Check the format and version.
		ModelFormat file_format
			(root.name(),
			 boost::lexical_cast<Version>(root.attribute("version")),
			 boost::lexical_cast<Version>(root.attribute("backto")));
		CHECK(file_format.GetName() == mFormat.GetName(),
			  "XML file contains the wrong type of data");
		CHECK(mFormat.GetVersion() >= file_format.GetCompatibleBackTo(),
			  "XML file is in a newer, unsupported format");
		CHECK(file_format.GetVersion() >= inEarliestFormat,
			  "XML file is in a older, unsupported format");
		mFormat = file_format;

		// Get our top-level map and fill it out.
		CHECK(root.attribute("type") == "map",
			  "Root element in XML document have type 'map'");
		mRoot->Fill(root);
	}
	catch (...)
	{
		if (doc)
			xmlFreeDoc(doc);
		throw;
	}

	xmlFreeDoc(doc);
	ClearUndoList();
}

Model::~Model()
{
	ClearRedoList();
	ClearUndoList();
}

bool Model::CanUndo()
{
	return mChangePosition != mChanges.begin();
}

void Model::Undo()
{
	ASSERT(CanUndo());
	(*--mChangePosition)->Revert();
}

void Model::ClearUndoList()
{
	ChangeList::iterator i = mChanges.begin();
	for (; i != mChangePosition; i = mChanges.erase(i))
	{
		(*i)->FreeResources();
		delete *i;
	}
	ASSERT(!CanUndo());
}

bool Model::CanRedo()
{
	return mChangePosition != mChanges.end();
}

void Model::Redo()
{
	ASSERT(CanRedo());
	(*mChangePosition++)->Apply();
}

void Model::ClearRedoList()
{
	// If there's no redo list, give up now.
	if (!CanRedo())
		return;

	// Walk the redo list backwards, destroying the Change objects
	// pointed to by each list element.
	ChangeList::iterator i = mChanges.end();
	do
	{
		--i;
		(*i)->FreeResources();
		delete *i;
		*i = NULL;
	} while (i != mChangePosition);

	// Free the actual list elements.
	mChangePosition = mChanges.erase(mChangePosition, mChanges.end());
	ASSERT(!CanRedo());	
}

void Model::ApplyChange(Change *inChange)
{
	ClearRedoList();
	inChange->Apply();
	mChangePosition = mChanges.insert(mChangePosition, inChange);
	mChangePosition++;
}

void Model::SaveAs(const std::string &inFile)
{
	// Create a tree.
	xmlDocPtr doc = xmlNewDoc(xml_node::to_utf8("1.0"));
	doc->children =
		xmlNewDocNode(doc, NULL, xml_node::to_utf8(mFormat.GetName().c_str()),
					  NULL);
	xml_node root(doc->children);
	std::string vers(boost::lexical_cast<std::string>(mFormat.GetVersion()));
	std::string back(boost::lexical_cast<std::string>(mFormat.GetCompatibleBackTo()));
	root.set_attribute("version", vers);
	root.set_attribute("backto", back);

	// Add nodes as appropriate.
	mRoot->Write(root);

	// Serialize it to a file.
	int result = xmlSaveFormatFile(inFile.c_str(), doc, 1);
	xmlFreeDoc(doc);
	CHECK(result != -1, "Failed to save XML file");
}