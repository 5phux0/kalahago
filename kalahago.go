package kalahago

/*
#include "node.h"
*/
import "C"
import (
	"bytes"
	"encoding/binary"
	"errors"
	"sync"
	"time"
	"unsafe"
)

type EVnode C.struct_evaluatedNode
type EVnodeState uint8

type Board struct {
	b   *C.struct_board
	ctx *C.struct_boardContext
}

type BoardState struct {
	Pholes, Oholes []int
	Pscore, Oscore int
	Mover          bool
}

const (
	State_NoWin EVnodeState = iota
	State_HasWon
	State_WinningTurn
	State_WillWin
)

var (
	Search_lock           sync.Mutex
	Search_limit          time.Time
	Search_limit_exceeded bool
)

func (node *EVnode) Free() {
	if node != nil {
		C.enode_free((*C.struct_evaluatedNode)(node))
	}
}

func (node *EVnode) GetWinningPath() (EVnodeState, []int, error) {
	if node == nil {
		return State_NoWin, []int{}, nil
	}
	if (*node).childrenLen == 0 {
		return State_HasWon, []int{}, nil
	}
	child := (*(*node).children)
	path, err := (*EVnode)(child).GetPath()
	if err != nil {
		return State_WillWin, []int{}, err
	}

	if (*child).childrenLen == 0 {
		return State_WinningTurn, path, nil
	} else {
		return State_WillWin, path, nil
	}
}

func (node *EVnode) GetPath() ([]int, error) {
	return intSliceFromCuintArray((*node).path, int(C.enode_getPathLen((*C.struct_evaluatedNode)(node))))
}

func BoardFromHoles(holes []int) Board {
	choles := make([]C.uint, len(holes))
	for i, v := range holes {
		//Check if strict conversion is possible
		choles[i] = C.uint(v)
	}
	b := Board{}
	C.newBoardAndContext(&b.b, &b.ctx, (*C.uint)(&choles[0]), C.uint(len(holes)))
	return b
}

func (b Board) Free() {
	C.board_free(b.b)
	C.free(unsafe.Pointer(b.ctx))
}

func (b Board) Copy() Board {
	cpy := Board{}
	cpy.b = C.board_copy(b.b, b.ctx)
	cpy.ctx = C.context_new((*b.ctx).nHoles, (*b.ctx).totalStones)
	return cpy
}

func (b Board) Pickup(index int) {
	C.board_pickup(b.b, C.int(index), b.ctx)
}

//export goOverseer
func goOverseer(ctx *C.struct_boardContext) C.bool {
	if time.Now().After(Search_limit) {
		Search_limit_exceeded = true
		return false
	}
	return true
}

func (b Board) SearchWinPath(limit time.Duration) (*EVnode, error) {
	mnode := C.mnode_new(C.board_copy(b.b, b.ctx), b.ctx)
	defer C.mnode_free(mnode)
	Search_lock.Lock()
	defer Search_lock.Unlock()
	Search_limit = time.Now().Add(limit)
	Search_limit_exceeded = false

	enode := C.searchWinPath(mnode, b.ctx)
	if Search_limit_exceeded {
		return (*EVnode)(enode), errors.New("Time limit exceeded")
	}
	return (*EVnode)(enode), nil
}

func (b Board) GetState() (BoardState, error) {
	var err error
	nholes := int((*b.ctx).nHoles)
	state := BoardState{nil, nil, int((*b.b).player), int((*b.b).opponent), bool((*b.b).mover)}

	state.Pholes, err = intSliceFromCuintArray((*b.b).pholes, nholes)
	if err != nil {
		return BoardState{}, err
	}
	state.Oholes, err = intSliceFromCuintArray((*b.b).oholes, nholes)
	if err != nil {
		return BoardState{}, err
	}

	return state, nil
}

func (b Board) GetMoveCount() int {
	return int((*b.ctx).moveCount)
}

func intSliceFromCuintArray(arrayPointer *C.uint, length int) ([]int, error) {
	arrayBytes := C.GoBytes(unsafe.Pointer(arrayPointer), C.int(length*int(unsafe.Sizeof(C.uint(0)))))
	reader := bytes.NewReader(arrayBytes)
	uintSlice := make([]C.uint, length)
	err := binary.Read(reader, binary.LittleEndian, &uintSlice)
	if err != nil {
		return []int{}, err
	}
	intSlice := make([]int, length)
	for i, v := range uintSlice {
		intSlice[i] = int(v)
	}
	return intSlice, err
}
