import gc
import torch
import logging
import traceback
from typing import Optional, Iterator, List, Any, Generator, AsyncGenerator, Dict, Union
from transformers import PreTrainedTokenizer
from maga_transformer.utils.util import get_mem_info
from maga_transformer.utils.time_util import Timer
from maga_transformer.config.exceptions import ExceptionType, FtRuntimeException
from maga_transformer.models.base_model import BaseModel, TokenizerBase, GenerateInput
from maga_transformer.config.generate_config import GenerateConfig
from maga_transformer.async_decoder_engine.decoder_engine import DecoderEngine
from maga_transformer.async_decoder_engine.engine_creator import create_engine
from maga_transformer.distribute.worker_info import g_parallel_info
from maga_transformer.models.base_model import GenerateOutput
from maga_transformer.async_decoder_engine.ptuning import get_ptuning_params

class AsyncModel:
    def __init__(self, model: BaseModel, sp_model: Optional[BaseModel] = None) -> None:
        self.model = model
        self.sp_model = sp_model
        self.config = model.config
        assert self.config.max_seq_len > 0
        self.tokenizer = model.tokenizer
        ptuning_args = get_ptuning_params(self.model, self.tokenizer)
        logging.info(f'first mem info: used:{get_mem_info().used} free: {get_mem_info().free}')
        if self.sp_model is not None:
            assert ptuning_args is None, "speculative don't support ptuning yet"
            self.decoder_engine_ = create_engine(self.model, self.config, None, self.sp_model, self.sp_model.config)
        else:
            self.decoder_engine_ = create_engine(model, self.config, ptuning_args)

    def is_multimodal(self) -> bool:
        return self.model.is_multimodal()

    @property
    def default_generate_config(self) -> GenerateConfig:
        return self.model.default_generate_config

    # just for perf test
    def enable_perf_test_schedule_strategy(self):
        self.decoder_engine_.scheduler_.enable_perf_test_schedule_strategy()

    def stop(self):
        self.decoder_engine_.stop()

    def update(self, lora_infos: Dict[str, str]):
        with Timer() as timer:
            self.decoder_engine_.executor_.model_ops.gpt_op.weight.lora_resource.update(lora_infos)
        logging.info(f'update lora weights time: {timer.cost_ms() / 1000 :.2f} s')

    @torch.no_grad()
    def enqueue(self, input: GenerateInput):
        if g_parallel_info.tp_size > 1 and g_parallel_info.tp_rank > 0:
            raise Exception('bug, not supposed to be here')
        return self.decoder_engine_.decode(input)