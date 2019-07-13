// "Project Butterfly" - CPSC 5300 / 4300 Summer 2019                                                  
// See README.md for details 
// THIS FILE: 

// Heap storage engine components
// Inherits and implements
// abstract base classes from storage_engine.h and heap_storage.h

#include "storage_engine.h"
#include "heap_storage.h"

typedef u_int16_t u16;

// Provided on Milestone 2 Canvas page
SlottedPage::SlottedPage(Dbt &block, BlockID block_id, bool is_new) : DbBlock(block, block_id, is_new) {
    if (is_new) {
        this->num_records = 0;
        this->end_free = DbBlock::BLOCK_SZ - 1;
        put_header();
    } else {
        get_header(this->num_records, this->end_free);
    }
}

// Add a new record to the block. Return its id.
// Provided on Milestone 2 Canvas page
RecordID SlottedPage::add(const Dbt* data) throw(DbBlockNoRoomError) {
    if (!has_room(data->get_size()))
        throw DbBlockNoRoomError("not enough room for new record");
    u16 id = ++this->num_records;
    u16 size = (u16) data->get_size();
    this->end_free -= size;
    u16 loc = this->end_free + 1;
    put_header();
    put_header(id, size, loc);
    std::memcpy(this->address(loc), data->get_data(), size);
    return id;
}
// Returns a Dbt of the given record
Dbt* SlottedPage::get(RecordID record_id){
  u16 size, loc;
  this->get_header(size, loc, record_id);
  if (loc == 0)
    return nullptr; //same as python, record was deleted
  char* rec = new char[size];
  memcpy(rec, address(loc), size);
  Dbt *data = new Dbt(rec, loc);
  return data;
}

// puts a record in at the given record id
// moves existing records as needed
void SlottedPage::put(RecordID record_id, const Dbt &data) throw(DbBlockNoRoomError)
{
  u16 size, loc;
  this->get_header(size, loc, record_id);
  u16 new_size = data->get_size();
  if (new_size > size){
    u16 extra = new_size - size;
    if (!this->has_room(extra))
      throw DbBlockNoRoomError("not enough room in block");
    this->slide(loc + new_size, loc + size);
    std::memcpy(address(loc - extra), data->get_data(), new_size);
  }
  else{
    std::memcpy(addres(loc), data->get_data(), new_size);
    this->slide(loc + new_size, loc + size);
  }
  this->get_header(size, loc, record_id);
  this->put_header(record_id, size, loc);
}

// just like the python:
// (comment from py file): "Mark the given id as deleted by changing 
// its size to zero and its location to 0.
// Compact the rest of the data in the block. But keep the record 
// ids the same for everyone."
void SlottedPage::del(RecordID record_id)
{
    u16 size, loc;
    this->get_header(size,loc,record_id);
    this->put_header(record_id,0,0);
    this->slide(loc, loc + size);
}

// Note need for function caller to free memory!
RecordIDs* SlottedPage::ids()
{
    RecordIDs* recs = new RecordIDs();
    for(RecordID i = 0; i < this->num_records; ++i) {
       // Only add IDs of non-deleted records
       u16 size, loc;
       this->get_header(size,loc,i);
       // should these "==" be "!="?
       if(size == 0 && loc == 0) // Is deleted
           recs->push_back(i);
    }
    return recs;
}

// TODO commenti
// TODO FIXME commented out in order to get a compile -
// fix and uncomment!
// get_n accepts only ONE u16 parameter!
// get_header, not get_n, worked on this with Lundeen
// so I think its right, sorry for lack of comments
void SlottedPage::get_header(u16 &size, u16 & loc, RecordID id)
{
    size = get_n(4*id, size);
    loc = get_n(4*id +2, size);
}


// 'FAKE' function to tesk compilation and linking:
/*
void SlottedPage::get_header(u16& size, u16& loc, RecordID id) {
    // do nothing real
    size = 0; loc = 0;
}
*/

// Store the size and offset for given id. For id of zero, store the block header.
// Provided on Milestone 2 Canvas page
void SlottedPage::put_header(RecordID id, u16 size, u16 loc) {
    if (id == 0) { // called the put_header() version and using the default params
        size = this->num_records;
        loc = this->end_free;
    }
    put_n(4*id, size);
    put_n(4*id + 2, loc);
}

// just like the python
bool SlottedPage::has_room(u16 size)
{
    return size <= (end_free - (num_records + 1)*4);
}

void SlottedPage::slide(u16 start, u16 end)
{
  u16 shift = end - start;
  if (shift == 0)
    return;
  // may need abs() around shift to stop errors?
  std::memcpy(address(this->end_free + 1), address(this->end_free + 1 + shift), shift);
  RecordIDs* ids = this->ids();
  for (u16 id : *ids){
    u16 size, loc;
    this->get_header(size, loc, id);
    if (loc <= start){
      loc += shift;
      put_header(id, size, loc);
    }
    this->end_free += shift;
    put_header();
  }
  delete ids;
}

// Get 2-byte integer at given offset in block.
// Provided on Milestone 2 Canvas page
u16 SlottedPage::get_n(u16 offset) {
    return *(u16*)this->address(offset);
}

// Put a 2-byte integer at given offset in block.
// Provided on Milestone 2 Canvas page
void SlottedPage::put_n(u16 offset, u16 n) {
    *(u16*)this->address(offset) = n;
}

// Make a void* pointer for a given offset into the data block.
// Provided on Milestone 2 Canas page
void* SlottedPage::address(u16 offset) {
    return (void*)((char*)this->block.get_data() + offset);
}


// Heapfile class
void HeapFile::create()
{
  this->db_open();
  SlottedPage* block = this->get_new();
  this->put(block);
}

void HeapFile::drop()
{
  this->close();
  remove(this->dbfilename.c_str());
}

void HeapFile::open()
{
  this->db_open();
}

void HeapFile::close()
{
  this->db.close();
  this->closed = true;
}

SlottedPage* HeapFile::get(BlockID block_id)
{
  
}

// Allocate a new block for the database file.
// Returns the new empty DbBlock that is managing the records in this block and its block id.
// Provided on Milestone 2 Canvas page
SlottedPage* HeapFile::get_new(void) {
    char block[DB_BLOCK_SZ];
    std::memset(block, 0, sizeof(block));
    Dbt data(block, sizeof(block));

    int block_id = ++this->last;
    Dbt key(&block_id, sizeof(block_id));

    // write out an empty block and read it back in so Berkeley DB is managing the memory
    SlottedPage* page = new SlottedPage(data, this->last, true);
    this->db.put(nullptr, &key, &data, 0); // write it out with initialization applied
    this->db.get(nullptr, &key, &data, 0);
    return page;
}

void HeapFile::put(DbBlock* block)
{
  BlockID id = block->get_block_id();
}

// returns a vector of all block ids
// unsure if this is correct
BlockIDs* HeapFile::block_ids()
{
  BlockIDs* pages = new BlockIDs();
  for(RecordID i = 0; i < this->last + 1; ++i)
    pages->push_back(i);
  return pages;    
}

void HeapFile::db_open(unsigned int flags)
{
  if(!this->closed)
    return;
  // TODO

}

/*
  HeapTable::HeapTable(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes) : DbRelation(table_name, column_names, column_attributes), file(table_name)
{

}
*/

void HeapTable::create()
{
  this->file.create();
}

void HeapTable::create_if_not_exists()
{
  try{
    this->file.open();
  } catch(int e){
    this->create();
  }
}

void HeapTable::drop()
{
  this->file.drop();
}

Handle HeapTable::insert(const ValueDict* row)
{

}

void HeapTable::update(const Handle handle, const ValueDict* new_values)
{

}

void HeapTable::del(const Handle handle)
{

}

Handles* HeapTable::select()
{

}

// TODO comment (already in header?)
// Provided on Milestone 2 Canvas page 
Handles* HeapTable::select(const ValueDict* where) {
    Handles* handles = new Handles();
    BlockIDs* block_ids = file.block_ids();
    for (auto const& block_id: *block_ids) {
        SlottedPage* block = file.get(block_id);
        RecordIDs* record_ids = block->ids();
        for (auto const& record_id: *record_ids)
            handles->push_back(Handle(block_id, record_id));
        delete record_ids;
        delete block;
    }
    delete block_ids;
    return handles;
}

ValueDict* HeapTable::project(Handle handle)
{

}

ValueDict* HeapTable::project(Handle handle, const ColumnNames* column_names)
{

}

ValueDict* HeapTable::validate(const ValueDict* row)
{

}

Handle HeapTable::append(const ValueDict* row)
{

}

// return the bits to go into the file
// caller responsible for freeing the returned Dbt and its enclosed ret->get_data().
// Provided on Milestone 2 Canvas page
Dbt* HeapTable::marshal(const ValueDict* row) {
    char *bytes = new char[DbBlock::BLOCK_SZ]; // more than we need (we insist that one row fits into DbBlock::BLOCK_SZ)
    uint offset = 0;
    uint col_num = 0;
    for (auto const& column_name: this->column_names) {
        ColumnAttribute ca = this->column_attributes[col_num++];
        ValueDict::const_iterator column = row->find(column_name);
        Value value = column->second;
        if (ca.get_data_type() == ColumnAttribute::DataType::INT) {
            *(int32_t*) (bytes + offset) = value.n;
            offset += sizeof(int32_t);
        } else if (ca.get_data_type() == ColumnAttribute::DataType::TEXT) {
            uint size = value.s.length();
            *(u16*) (bytes + offset) = size;
            offset += sizeof(u16);
            std::memcpy(bytes+offset, value.s.c_str(), size); // assume ascii for now
            offset += size;
        } else {
            throw DbRelationError("Only know how to marshal INT and TEXT");
        }
    }
    char *right_size_bytes = new char[offset];
    memcpy(right_size_bytes, bytes, offset);
    delete[] bytes;
    Dbt *data = new Dbt(right_size_bytes, offset);
    return data;
}

ValueDict* HeapTable::unmarshal(Dbt* data)
{

}

// test function -- returns true if all tests pass
// Provided on Milestone 2 Canvas page
bool test_heap_storage() {
    ColumnNames column_names;
    column_names.push_back("a");
    column_names.push_back("b");
    ColumnAttributes column_attributes;
    ColumnAttribute ca(ColumnAttribute::INT);
    column_attributes.push_back(ca);
    ca.set_data_type(ColumnAttribute::TEXT);
    column_attributes.push_back(ca);
    HeapTable table1("_test_create_drop_cpp", column_names, column_attributes);
    table1.create();
    std::cout << "create ok" << std::endl;
    table1.drop();  // drop makes the object unusable because of BerkeleyDB restriction -- maybe want to fix this some day
        std::cout << "drop ok" << std::endl;
    HeapTable table("_test_data_cpp", column_names, column_attributes);
    table.create_if_not_exists();
    std::cout << "create_if_not_exsts ok" << std::endl;
    ValueDict row;
    row["a"] = Value(12);
    row["b"] = Value("Hello!");
    std::cout << "try insert" << std::endl;
    table.insert(&row);
    std::cout << "insert ok" << std::endl;
    Handles* handles = table.select();
    std::cout << "select ok " << handles->size() << std::endl;
    ValueDict *result = table.project((*handles)[0]);
    std::cout << "project ok" << std::endl;
    Value value = (*result)["a"];
    if (value.n != 12)
        return false;
    value = (*result)["b"];
    if (value.s != "Hello!")
        return false;
    table.drop();
    return true;
}
