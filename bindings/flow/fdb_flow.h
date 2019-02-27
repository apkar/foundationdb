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

	class DatabaseContext : public ReferenceCounted<DatabaseContext>, NonCopyable {
		friend class Transaction;
	public:
		~DatabaseContext() {
			fdb_database_destroy( db );
		}

		void setDatabaseOption(FDBDatabaseOption option, Optional<StringRef> value = Optional<StringRef>());

	private:
		FDBDatabase* db;
		explicit DatabaseContext( FDBDatabase* db ) : db(db) {}

		friend class API;
	};

	class API {
	public:
		static API* selectAPIVersion(int apiVersion);
		static API* getInstance();
		static bool isAPIVersionSelected();

		void setNetworkOption(FDBNetworkOption option, Optional<StringRef> value = Optional<StringRef>());

		void setupNetwork();
		void runNetwork();
		void stopNetwork();

		Reference<DatabaseContext> createDatabase( std::string const& connFilename="" );

		bool evaluatePredicate(FDBErrorPredicate pred, Error const& e);
		int getAPIVersion() const;

	private:
		static API* instance;

		API(int version);
		int version;
	};

	class Transaction : public ITransaction, ReferenceCounted<Transaction>, private NonCopyable, public FastAllocated<Transaction> {
	public:
		explicit Transaction( Reference<DatabaseContext> const& db );
		virtual ~Transaction() {
			if (tr) {
				fdb_transaction_destroy(tr);
			}
		}

		void setVersion( Version v ) override;
		Future<Version> getReadVersion() override;

		Future< Optional<FDBStandalone<ValueRef>> > get( const Key& key, bool snapshot = false ) override;
		Future< Void > watch( const Key& key ) override;
		Future< FDBStandalone<KeyRef> > getKey( const KeySelector& key, bool snapshot = false ) override;
		using ITransaction::getRange;
		Future< FDBStandalone<RangeResultRef> > getRange( const KeySelector& begin, const KeySelector& end, GetRangeLimits limits = GetRangeLimits(), bool snapshot = false, bool reverse = false, FDBStreamingMode streamingMode = FDB_STREAMING_MODE_SERIAL) override;

		// Future< Standalone<VectorRef<const char*>> > getAddressesForKey(const Key& key);

		void addReadConflictRange( KeyRangeRef const& keys ) override;
		void addReadConflictKey( KeyRef const& key ) override;
		void addWriteConflictRange( KeyRangeRef const& keys ) override;
		void addWriteConflictKey( KeyRef const& key ) override;
		// void makeSelfConflicting() { tr.makeSelfConflicting(); }

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
		// double getBackoff() { return tr.getBackoff(); }
		// void debugTransaction(UID dID) { tr.debugTransaction(dID); }

		Transaction() : tr(NULL) {}
		Transaction( Transaction&& r ) noexcept(true) {
			tr = r.tr;
			r.tr = NULL;
		}
		Transaction& operator=( Transaction&& r ) noexcept(true) {
			tr = r.tr;
			r.tr = NULL;
			return *this;
		}

		void addref() override { ReferenceCounted<Transaction>::addref(); }
		void delref() override { ReferenceCounted<Transaction>::delref(); }

	private:
		FDBTransaction* tr;
	};

}

#endif
