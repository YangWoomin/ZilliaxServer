package main

import (
	"context"

	"github.com/redis/go-redis/v9"
	"go.uber.org/zap"
)

type MsgAggr struct {
	sugar *zap.SugaredLogger
	id    int
	ctp   string

	rcc *redis.ClusterClient

	ttl int
}

func (ma MsgAggr) GetId() int {
	return ma.id
}

func (ma MsgAggr) GetConsTopic() string {
	return ma.ctp
}

func (ma MsgAggr) GetProdTopic() string {
	return ""
}

func (ma MsgAggr) Process(fr *FetchResult, ig *bool) bool {

	sugar := ma.sugar

	script := `

if (#KEYS ~= 1 or #ARGV ~= 3) then return 0 end
local key = "message:" .. KEYS[1]
local cid = ARGV[1]
local msg_sn = ARGV[2]
local ttl = tonumber(ARGV[3])
local dup_check_key = key .. ":" .. cid .. ":" .. msg_sn
local msg_cnt_key = key .. ":msg_cnt"

local check = tonumber(redis.call("GET", dup_check_key) or '0')
if (check == 0) then
	redis.call("INCR", msg_cnt_key)
	redis.call("SET", dup_check_key, 1, "EX", ttl)
	return 1
else
	return 2
end

return 0

	`

	ctx := context.Background()
	key := "{" + string(fr.msg.Value) + "}"
	res, err := ma.rcc.Eval(ctx, script, []string{key}, fr.cid, fr.sn, ma.ttl).Result()
	if err != nil {
		sugar.Errorf("executing script failed, id : %d, cid : %s, sn : %s, err : %s",
			ma.id, fr.cid, fr.sn, err.Error())

		return false
	}

	ret, ok := res.(int64)
	if !ok {
		sugar.Errorf("getting result from script failed, id : %d, cid : %s, sn : %s, res : %T",
			ma.id, fr.cid, fr.sn, res)

		*ig = true
		return false
	}

	if ret == 0 {
		// TODO: save this message to DLQ
		sugar.Errorf("invalid script result, id : %d, cid : %s, sn : %s",
			ma.id, fr.cid, fr.sn)
		*ig = true
	} else if ret == 2 {
		sugar.Warnf("duplcated message occurred, id : %d, cid : %s, sn : %s",
			ma.id, fr.cid, fr.sn)
	} else {
		// sugar.Infof("executing script succeeded, id : %d, cid : %s, sn : %s, ret : %d",
		// 	ma.id, fr.cid, fr.sn, ret)
	}

	return true
}
