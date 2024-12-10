package main

import (
	"context"

	"github.com/redis/go-redis/v9"
	"go.uber.org/zap"
)

type ClntMsgCnter struct {
	sugar *zap.SugaredLogger
	id    int
	ctp   string
	ptp   string

	rcc *redis.ClusterClient
}

func (cmc ClntMsgCnter) GetId() int {
	return cmc.id
}

func (cmc ClntMsgCnter) GetConsTopic() string {
	return cmc.ctp
}

func (cmc ClntMsgCnter) GetProdTopic() string {
	return cmc.ptp
}

func (cmc ClntMsgCnter) Process(fr *FetchResult) bool {

	sugar := cmc.sugar

	script := `

if (#KEYS ~= 1 or #ARGV ~= 1) then return 0 end
local key = "consumer:" .. KEYS[1]
local last_msg_sn_key = key .. ":last_msg_sn"
local msg_cnt_key = key .. ":msg_cnt"
local msg_sn = tonumber(ARGV[1])

local last_msg_sn = tonumber(redis.call("GET", last_msg_sn_key) or '0')
if last_msg_sn + 1 ~= msg_sn then
	return 0
end

redis.call("INCR", last_msg_sn_key)
redis.call("INCR", msg_cnt_key)
return 1

	`

	ctx := context.Background()
	key := "{" + fr.cid + "}"
	res, err := cmc.rcc.Eval(ctx, script, []string{key}, fr.sn).Result()
	if err != nil {
		sugar.Errorf("executing script failed, id : %d, cid : %s, sn : %s, err : %s",
			cmc.id, fr.cid, fr.sn, err.Error())

		return false
	}

	if res == 0 {
		sugar.Errorf("invalid script result, id : %d, cid : %s, sn : %s",
			cmc.id, fr.cid, fr.sn)
		return false
	}

	sugar.Infof("executing script succeeded, id : %d, cid : %s, sn : %s",
		cmc.id, fr.cid, fr.sn)

	return true
}
