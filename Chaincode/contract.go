package main

import (
	"encoding/json"
	"fmt"
	"github.com/hyperledger/fabric-contract-api-go/v2/contractapi"
	"strconv"
)

type SensorReading struct {
	UUID        string  `json:"uuid"`
	Timestamp   uint64  `json:"timestamp"`
	Pressure    float64 `json:"pressure"`
	Humidity    float64 `json:"humidity"`
	Temperature float64 `json:"temperature"`
	R           uint8   `json:"r"`
	G           uint8   `json:"g"`
	B           uint8   `json:"b"`
	TVOC        uint16  `json:"tvoc"`
	AccelX      int8    `json:"accel_x"`
	AccelY      int8    `json:"accel_y"`
	AccelZ      int8    `json:"accel_z"`
}

type SmartContract struct{ contractapi.Contract }

// key = reading~uuid~timestamp for fast range scans
func (s *SmartContract) CreateReading(ctx contractapi.TransactionContextInterface,
	uuid string, ts uint64, jsonBlob string) error {

	key, _ := ctx.GetStub().
		CreateCompositeKey("reading", []string{uuid, strconv.FormatUint(ts, 10)})

	if v, _ := ctx.GetStub().GetState(key); v != nil {
		return fmt.Errorf("reading exists")
	}
	return ctx.GetStub().PutState(key, []byte(jsonBlob))
}

func (s *SmartContract) GetReading(ctx contractapi.TransactionContextInterface,
	uuid string, ts uint64) (*SensorReading, error) {

	key, _ := ctx.GetStub().
		CreateCompositeKey("reading", []string{uuid, strconv.FormatUint(ts, 10)})

	val, err := ctx.GetStub().GetState(key)
	if err != nil || val == nil {
		return nil, fmt.Errorf("not found")
	}

	var out SensorReading
	_ = json.Unmarshal(val, &out)
	return &out, nil
}

func (s *SmartContract) QueryDevice(ctx contractapi.TransactionContextInterface,
	uuid string) ([]*SensorReading, error) {

	it, err := ctx.GetStub().
		GetStateByPartialCompositeKey("reading", []string{uuid})
	if err != nil {
		return nil, err
	}
	defer it.Close()

	var list []*SensorReading
	for it.HasNext() {
		kv, _ := it.Next()
		var r SensorReading
		_ = json.Unmarshal(kv.Value, &r)
		list = append(list, &r)
	}
	return list, nil
}

func (s *SmartContract) DeleteReading(
	ctx contractapi.TransactionContextInterface,
	uuid string, ts uint64,
) error {

	key, _ := ctx.GetStub().
		CreateCompositeKey("reading",
			[]string{uuid, strconv.FormatUint(ts, 10)}) // build the same key

	// Check the record really exists – saves silent no-op deletes.
	val, err := ctx.GetStub().GetState(key)
	if err != nil {
		return err
	}
	if val == nil {
		return fmt.Errorf("reading %s not found", key)
	}

	return ctx.GetStub().DelState(key) // physical delete in world-state DB
}

func main() {
	cc, _ := contractapi.NewChaincode(&SmartContract{})
	panic(cc.Start())
}
