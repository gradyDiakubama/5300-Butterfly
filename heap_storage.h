/**
 * @file heap_storage.h - Implementation of storage_engine with a heap file structure.
 * SlottedPage: DbBlock
 * HeapFile: DbFile
 * HeapTable: DbRelation
 *
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5300, Summer 2018"
 */
#pragma once

#include <cstring> // for memcpy, memset
#include "db_cxx.h"
#include "storage_engine.h"

/**
 * @class SlottedPage - heap file implementation of DbBlock.
 *
 *      Manage a database block that contains several records.
        Modeled after slotted-page from Database Systems Concepts, 6ed, Figure 10-9.

        Record id are handed out sequentially starting with 1 as records are added with add().
        Each record has a header which is a fixed offset from the beginning of the block:
            Bytes 0x00 - Ox01: number of records
            Bytes 0x02 - 0x03: offset to end of free space
            Bytes 0x04 - 0x05: size of record 1
            Bytes 0x06 - 0x07: offset to record 1
            etc.
 *
 */
class SlottedPage : public DbBlock {
public:
	SlottedPage(Dbt &block, BlockID block_id, bool is_new=false);
	// Big 5 - we only need the destructor, copy-ctor, move-ctor, and op= are unnecessary
	// but we delete them explicitly just to make sure we don't use them accidentally
	virtual ~SlottedPage() {}
	SlottedPage(const SlottedPage& other) = delete;
	SlottedPage(SlottedPage&& temp) = delete;
	SlottedPage& operator=(const SlottedPage& other) = delete;
	SlottedPage& operator=(SlottedPage& temp) = delete;

	virtual RecordID add(const Dbt* data) throw(DbBlockNoRoomError);
	virtual Dbt* get(RecordID record_id);
	virtual void put(RecordID record_id, const Dbt &data) throw(DbBlockNoRoomError);
	virtual void del(RecordID record_id);
	virtual RecordIDs* ids(void);

protected:
	u_int16_t num_records;
	u_int16_t end_free;

    // Get a record header (size and location, via ref args) from a record number
	virtual void get_header(u_int16_t &size, u_int16_t &loc, RecordID id=0);
	
    // Put a record header for a given record id
    virtual void put_header(RecordID id=0, u_int16_t size=0, u_int16_t loc=0);
	
    virtual bool has_room(u_int16_t size);
	
    virtual void slide(u_int16_t start, u_int16_t end);
	
    // Get u16 at given offset in block
    virtual u_int16_t get_n(u_int16_t offset);
	
    // Put u16 at given offset in block
    virtual void put_n(u_int16_t offset, u_int16_t n);
	
    virtual void* address(u_int16_t offset);
};

/**
 * @class HeapFile - heap file implementation of DbFile
 *
 * Heap file organization. Built on top of Berkeley DB RecNo file. There is one of our
        database blocks for each Berkeley DB record in the RecNo file. In this way we are using Berkeley DB
        for buffer management and file management.
        Uses SlottedPage for storing records within blocks.
 */
class HeapFile : public DbFile {
public:
    // Not init last to 1 - BerkelyDB record numbers start at 1!
    // https://docs.oracle.com/cd/E17276_01/html/programmer_reference/am_conf_logrec.html
	HeapFile(std::string name) : DbFile(name), dbfilename(""), last(1), closed(true), db(_DB_ENV, 0) {
        //const char* dbPath;
        //_DB_ENV->get_home(&dbPath); // TODO fixme! - correct '/' handling
        //dbfilename += dbPath;
        dbfilename += name;
        std::cout << "dbPathStr:" << dbfilename << std::endl;
    } // TODO move body to .cpp file
	virtual ~HeapFile() {}
	HeapFile(const HeapFile& other) = delete;
	HeapFile(HeapFile&& temp) = delete;
	HeapFile& operator=(const HeapFile& other) = delete;
	HeapFile& operator=(HeapFile&& temp) = delete;

	virtual void create(void);
	virtual void drop(void);
	virtual void open(void);
	virtual void close(void);
	
    // This function uses Kevins 'hack' to let BerkeleyDB take care of memory managment
    virtual SlottedPage* get_new(void);
	
    virtual SlottedPage* get(BlockID block_id);
	virtual void put(DbBlock* block);
	virtual BlockIDs* block_ids();

	virtual u_int32_t get_last_block_id() {return last;}

protected:
	std::string dbfilename;
	u_int32_t last;
	bool closed;
	Db db;
	virtual void db_open(uint flags=0);
};

/**
 * @class HeapTable - Heap storage engine (implementation of DbRelation)
 */

class HeapTable : public DbRelation {
public:
	// This is where the ctor issue was ... TODO
    HeapTable(Identifier table_name, ColumnNames column_names, ColumnAttributes column_attributes ) : 
        DbRelation(table_name,column_names,column_attributes), file(table_name) {} // TODO what needs done for ctor?
	virtual ~HeapTable(); // {} TODO: {} was given - implies no body needed, or...?
	HeapTable(const HeapTable& other) = delete;
	HeapTable(HeapTable&& temp) = delete;
	HeapTable& operator=(const HeapTable& other) = delete;
	HeapTable& operator=(HeapTable&& temp) = delete;

	virtual void create();
	virtual void create_if_not_exists();
	virtual void drop();

	virtual void open();
	virtual void close();

	virtual Handle insert(const ValueDict* row);
	virtual void update(const Handle handle, const ValueDict* new_values);
	virtual void del(const Handle handle);

	virtual Handles* select();
	virtual Handles* select(const ValueDict* where);
	virtual ValueDict* project(Handle handle);
	virtual ValueDict* project(Handle handle, const ColumnNames* column_names);

protected:
	HeapFile file;
	virtual ValueDict* validate(const ValueDict* row);
	virtual Handle append(const ValueDict* row);
	virtual Dbt* marshal(const ValueDict* row);
	virtual ValueDict* unmarshal(Dbt* data);
};

bool test_heap_storage();

