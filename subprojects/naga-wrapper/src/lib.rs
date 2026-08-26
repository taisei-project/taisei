#![crate_name = "wgslcrap"]

use std::error::Error;
use std::ffi::{c_char, c_void};
use std::ptr;
use std::slice;

use naga::back::wgsl::WriterFlags;
use naga::valid::ValidationFlags;
use spirv_webgpu_transform::{u8_slice_to_u32_vec, u32_slice_to_u8_vec};

pub type AllocFn = unsafe extern "C" fn(size: usize, arg: *mut c_void) -> *mut c_void;

#[repr(C)]
pub struct Allocaor {
	func: AllocFn,
	arg: *mut c_void,
}

impl Allocaor {
	pub unsafe fn copy_str(&self, s: &str) -> *mut c_char {
		let alloc_size = s.len() + 1;
		let buffer = (self.func)(alloc_size, self.arg) as *mut u8;

		if buffer.is_null() {
			return ptr::null_mut();
		}

		ptr::copy_nonoverlapping(s.as_ptr(), buffer, s.len());
		*buffer.add(s.len()) = 0;

		return buffer as *mut c_char;
	}
}

#[repr(C)]
pub struct WGSLResult {
	content_size: usize,
	content: *mut c_char,
	error: *mut c_char,
}

fn format_error_chain(err: &dyn Error) -> String {
	let mut result = err.to_string();
	let mut current = err.source();

	while let Some(cause) = current {
		result.push_str("\n  ↳ ");
		result.push_str(&cause.to_string());
		current = cause.source();
	}

	result
}

impl WGSLResult {
	pub fn content(allocator: &Allocaor, content: &str) -> Self {
		WGSLResult {
			content_size: content.len(),
			content: unsafe { allocator.copy_str(content) },
			error: ptr::null_mut(),
		}
	}

	pub fn error(allocator: &Allocaor, err: &str) -> Self {
		WGSLResult {
			content_size: 0,
			content: ptr::null_mut(),
			error: unsafe { allocator.copy_str(err) },
		}
	}
}

#[no_mangle]
pub unsafe extern "C" fn spirv_to_wgsl(
	code: *const u32,
	code_size: u32,
	allocator: Allocaor,
) -> WGSLResult {
	let byte_len = (code_size as usize) * std::mem::size_of::<u32>();
	let spirv_bytes: &[u8] = slice::from_raw_parts(code as *const u8, byte_len);

	let spirv_split: &[u8] = match spirv_webgpu_transform::combimgsampsplitter(
		&u8_slice_to_u32_vec(spirv_bytes), &mut Default::default()
	) {
		Ok(s) => &u32_slice_to_u8_vec(&s),
		Err(_) =>
			return WGSLResult::error(&allocator,
				"spirv_webgpu_transform::combimgsampsplitter() failed")
	};

	let module = match naga::front::spv::parse_u8_slice(spirv_split, &naga::front::spv::Options {
		adjust_coordinate_space: false,
		strict_capabilities: false,
		block_ctx_dump_prefix: None,
	}) {
		Ok(module) => module,
		Err(err) => {
			let errstr = format_error_chain(&err);
			return WGSLResult::error(&allocator,
				&format!("naga::front::spv::parse_u8_slice() failed: {errstr}"))
		},
	};

	let info = match naga::valid::Validator::new(
			ValidationFlags::all(),
			naga::back::wgsl::supported_capabilities())
		.subgroup_stages(naga::valid::ShaderStages::all())
		.subgroup_operations(naga::valid::SubgroupOperationSet::all())
		.validate(&module)
	{
		Ok(info) => info,
		Err(err) => {
			let errstr = format_error_chain(&err);
			return WGSLResult::error(&allocator,
				&format!("naga::valid::Validator::validate() failed: {errstr}"))
		}
	};

	let wgsl = match naga::back::wgsl::write_string(&module, &info, WriterFlags::EXPLICIT_TYPES) {
		Ok(s) => s,
		Err(err) => {
			let errstr = format_error_chain(&err);
			return WGSLResult::error(&allocator,
				&format!("naga::back::wgsl::write_string() failed: {errstr}"))
		}
	};

	return WGSLResult::content(&allocator, &wgsl);
}
