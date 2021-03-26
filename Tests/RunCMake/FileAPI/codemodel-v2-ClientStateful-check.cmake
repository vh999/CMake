set(expect
  query
  query/client-foo
  query/client-foo/query.json
  reply
  reply/codemodel-v2-[0-9a-f]+\\.json
  .*
  reply/index-[0-9.T-]+\\.json
  .*
  )
check_api("^${expect}$")

check_python(codemodel-v2)
