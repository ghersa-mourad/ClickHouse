#include <DB/Interpreters/ClusterProxy/AlterQueryConstructor.h>
#include <DB/Interpreters/InterpreterAlterQuery.h>
#include <DB/DataStreams/RemoteBlockInputStream.h>
#include <DB/DataStreams/LazyBlockInputStream.h>

namespace DB
{

namespace
{

constexpr PoolMode pool_mode = PoolMode::GET_ONE;

}

namespace ClusterProxy
{

BlockInputStreamPtr AlterQueryConstructor::createLocal(ASTPtr query_ast, const Context & context, const Cluster::Address & address)
{
	/// The ALTER query may be a resharding query that is a part of a distributed
	/// job. Since the latter heavily relies on synchronization among its participating
	/// nodes, it is very important to defer the execution of a local query so as
	/// to prevent any deadlock.
	auto interpreter = std::make_shared<InterpreterAlterQuery>(query_ast, context);
	auto stream = std::make_shared<LazyBlockInputStream>(
		[interpreter]() mutable
		{
			return interpreter->execute().in;
		});
	return stream;
}

BlockInputStreamPtr AlterQueryConstructor::createRemote(IConnectionPool * pool, const std::string & query,
	const Settings & settings, ThrottlerPtr throttler, const Context & context)
{
	auto stream = std::make_shared<RemoteBlockInputStream>(pool, query, &settings, throttler);
	stream->setPoolMode(pool_mode);
	return stream;
}

BlockInputStreamPtr AlterQueryConstructor::createRemote(ConnectionPoolsPtr & pools, const std::string & query,
	const Settings & settings, ThrottlerPtr throttler, const Context & context)
{
	auto stream = std::make_shared<RemoteBlockInputStream>(pools, query, &settings, throttler);
	stream->setPoolMode(pool_mode);
	return stream;
}

PoolMode AlterQueryConstructor::getPoolMode() const
{
	return pool_mode;
}

}

}
