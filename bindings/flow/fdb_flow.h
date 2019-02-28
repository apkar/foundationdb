/*
 * fdb_flow.h
 *
 * This source file is part of the FoundationDB open source project
 *
 * Copyright 2013-2018 Apple Inc. and the FoundationDB project authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FDB_FLOW_FDB_FLOW_H
#define FDB_FLOW_FDB_FLOW_H

#include "fdb_flow_api.h"

namespace FDB {

	class DatabaseContext : public IDatabase, NonCopyable {
	public:
		virtual ~DatabaseContext() {
			fdb_database_destroy( db );
		}

		Reference<Transaction> createTransaction() override;
		void setDatabaseOption(FDBDatabaseOption option, Optional<StringRef> value = Optional<StringRef>()) override;

//		void addref() override { ReferenceCounted<DatabaseContext>::addref(); }
//		void delref() override { ReferenceCounted<DatabaseContext>::delref(); }

	private:
		FDBDatabase* db;
		explicit DatabaseContext( FDBDatabase* db ) : db(db) {}

		friend class API;
	};

	class TransactionImpl : public Transaction, private NonCopyable, public FastAllocated<TransactionImpl> {
		friend class DatabaseContext;
	public:
		virtual ~TransactionImpl() {
			if (tr) {
				fdb_transaction_destroy(tr);
			}
		}

		void setReadVersion( Version v ) override;
		Future<Version> getReadVersion() override;

		Future< Optional<FDBStandalone<ValueRef>> > get( const Key& key, bool snapshot = false ) override;
		Future< FDBStandalone<KeyRef> > getKey( const KeySelector& key, bool snapshot = false ) override;

		Future< Void > watch( const Key& key ) override;

		using Transaction::getRange;
		Future< FDBStandalone<RangeResultRef> > getRange( const KeySelector& begin, const KeySelector& end, GetRangeLimits limits = GetRangeLimits(), bool snapshot = false, bool reverse = false, FDBStreamingMode streamingMode = FDB_STREAMING_MODE_SERIAL) override;

		void addReadConflictRange( KeyRangeRef const& keys ) override;
		void addReadConflictKey( KeyRef const& key ) override;
		void addWriteConflictRange( KeyRangeRef const& keys ) override;
		void addWriteConflictKey( KeyRef const& key ) override;

		void atomicOp( const KeyRef& key, const ValueRef& operand, FDBMutationType operationType ) override;
		void set( const KeyRef& key, const ValueRef& value ) override;
		void clear( const KeyRangeRef& range ) override;
		void clear( const KeyRef& key ) override;

		Future<Void> commit() override;
		Version getCommittedVersion() override;
		Future<FDBStandalone<StringRef>> getVersionstamp() override;

		void setOption( FDBTransactionOption option, Optional<StringRef> value = Optional<StringRef>() ) override;

		Future<Void> onError( Error const& e ) override;

		void cancel() override;
		void reset() override;

		TransactionImpl() : tr(NULL) {}
		TransactionImpl( TransactionImpl&& r ) noexcept(true) {
			tr = r.tr;
			r.tr = NULL;
		}
		TransactionImpl& operator=( TransactionImpl&& r ) noexcept(true) {
			tr = r.tr;
			r.tr = NULL;
			return *this;
		}

//		void addref() override { ReferenceCounted<TransactionImpl>::addref(); }
//		void delref() override { ReferenceCounted<TransactionImpl>::delref(); }

	private:
		FDBTransaction* tr;

		// Review: Do we need to hold reference on the database object? If so we may want to pass Reference to
		// DatabaseContext which holds FDBDatabase.
		explicit TransactionImpl(FDBDatabase *db);
	};

}

#endif
