#include "pch.h"

namespace invader {

ZSynth::ZSynth(ZVMProgram* program)
	: vm(program, new ZVMStack((uintptr_t)zalignedalloc(10*1024*1024, 16)), new ZVMStorage((uintptr_t)zalignedalloc(program->globalStorageSize, 16)))
	, program(program)
	, numInstruments(program->numSections-1)
{
	instruments = new ZInstrument*[numInstruments];

	for (uint32_t i=0; i<numInstruments; i++)
	{
		instruments[i] = new ZInstrument(this, program, program->GetInstrumentSectionID(i), vm.globalStorage);
	}

	for (uint32_t i=0; i<kNumMIDIChannels; i++)
		midiChannelToInstrumentMap[i] = nullptr;

	section = program->GetSynthSectionID(); 

	vm.CreateNodeInstances(program, section); // last section is master section

	vm.synth      = this;
	vm.instrument = nullptr;
	vm.voice      = nullptr;

	bytecodeStart = program->sections[section];
	bytecodeEnd   = program->sections[section+1];
}

ZSynth::~ZSynth(void)
{
	for (uint32_t i=0; i<numInstruments; i++)
		delete instruments[i];

	delete instruments;
	
	zalignedfree((void*)vm.globalStorage->mem);
	delete vm.globalStorage;

	zalignedfree((void*)vm.stack->mem);
	delete vm.stack;
}

void ZSynth::NoteOn(uint32_t channel, uint32_t note, uint32_t velocity, uint32_t deltaSamples)
{
	GetInstrumentFromMIDI(channel, note)->NoteOn(note, velocity, deltaSamples);
}

void ZSynth::NoteOff(uint32_t channel, uint32_t note, uint32_t deltaSamples)
{
	GetInstrumentFromMIDI(channel, note)->NoteOff(note, deltaSamples);
}

void ZSynth::ControlChange(uint32_t channel, uint32_t number, uint32_t value, uint32_t deltaSamples)
{
	channel; number; value; deltaSamples;
	//GetInstrumentFromMIDI(channel, note)->ControlChange(number, value, deltaSamples);
}

void ZSynth::ProcessBlock(void)
{
	for (uint32_t i=0; i<numInstruments; i++)
	{
		instruments[i]->sync = sync;
		instruments[i]->ProcessBlock();
	}

	vm.Run(bytecodeStart, program);

	sync.AdvanceBlock();
}

} // namespace invader
