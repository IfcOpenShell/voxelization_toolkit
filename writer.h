#ifndef WRITER_H
#define WRITER_H

#include "storage.h"
#include "H5Cpp.h"


#include <BRepPrimAPI_MakeBox.hxx>
#include <string>
#include <fstream>


#define BEGIN_LOOP(i0, i1, j0, j1, k0, k1) {\
	vec_n<3, typename std::remove_reference<decltype(i0)>::type> ijk;\
	for (ijk.get(0) = i0; ijk.get(0) < i1; ++ijk.get(0)) {\
		for (ijk.get(1) = j0; ijk.get(1) < j1; ++ijk.get(1)) {\
			for (ijk.get(2) = k0; ijk.get(2) < k1; ++ijk.get(2)) {

#define BEGIN_LOOP_I(i0, i1, j0, j1, k0, k1) {\
	vec_n<3, typename std::remove_reference<decltype(i0)>::type> ijk;\
	for (ijk.get(0) = i0; ijk.get(0) <= i1; ++ijk.get(0)) {\
		for (ijk.get(1) = j0; ijk.get(1) <= j1; ++ijk.get(1)) {\
			for (ijk.get(2) = k0; ijk.get(2) <= k1; ++ijk.get(2)) {

#define BEGIN_LOOP2(v0, v1) BEGIN_LOOP(v0.template get<0>(), v1.template get<0>(), v0.template get<1>(), v1.template get<1>(), v0.template get<2>(), v1.template get<2>())

#define BEGIN_LOOP_I2(v0, v1) BEGIN_LOOP_I(v0.template get<0>(), v1.template get<0>(), v0.template get<1>(), v1.template get<1>(), v0.template get<2>(), v1.template get<2>())

#define BEGIN_LOOP_ZERO_2(v1) BEGIN_LOOP2(make_vec<decltype(v1)::element_type>((decltype(v1)::element_type)0, (decltype(v1)::element_type)0, (decltype(v1)::element_type)0), v1)

#define END_LOOP }}}}


class abstract_writer {

public:
	abstract_voxel_storage* voxels_;
	void SetVoxels(abstract_voxel_storage* voxels) {
		voxels_ = voxels;
	}

	virtual void Write(const std::string& fnc) = 0;


};

class voxel_writer :public abstract_writer {
private:

	std::ofstream& assert_good_(const std::string& fn, std::ofstream& fs) {
		if (!fs.good()) {
			throw std::runtime_error("Unable to open file for writing. " + fn);
		}
		return fs;
	}

public:
	void Write(const std::string& fnc) {
		{
			std::string fn = fnc + std::string(".index");
			std::ofstream fs(fn.c_str());
			voxels_->write(file_part_index, assert_good_(fn, fs));
		}
		{
			std::string fn = fnc + std::string(".contents");
			std::ofstream fs(fn.c_str(), std::ios::binary);
			voxels_->write(file_part_contents, assert_good_(fn, fs));
		}
		{
			std::string fn = fnc + std::string(".meta");
			std::ofstream fs(fn.c_str());
			voxels_->write(file_part_meta, assert_good_(fn, fs));
		}
		{
			std::string fn = fnc + std::string(".primitives");
			std::ofstream fs(fn.c_str());
			voxels_->write(file_part_primitives, assert_good_(fn, fs));
		}
	}
};


class hdf_writer :public abstract_writer {

public:
	void Write(const std::string& fnc) {

		chunked_voxel_storage<bit_t>* storage = (chunked_voxel_storage<bit_t>*)voxels_;

		int continuous_count = 0;
		int planar_count = 0;
		int constant_count = 0;

		hsize_t nchunks_x = storage->num_chunks().get(0);
		hsize_t nchunks_y = storage->num_chunks().get(1);
		hsize_t nchunks_z = storage->num_chunks().get(2);

		BEGIN_LOOP(size_t(0), nchunks_x, 0U, nchunks_y, 0U, nchunks_z)
			auto c = storage->get_chunk(ijk);

		if (c == nullptr) {
			std::cout << "Null pointer" << std::endl;
			constant_count++;
		}

		else {
			if (c->is_explicit() || c->is_constant()) {
				if (c->is_constant()) {
					std::cout << "Count Constant chunk" << std::endl;
					constant_count++;
				}
				else {
					std::cout << "Count Continuous chunk" << std::endl;
					continuous_count++;
				}
			}

			else {

				std::cout << " Count Planar chunk" << std::endl;
				planar_count++;
			}
		}
		END_LOOP;


		const H5std_string FILE_NAME(fnc);
		H5::H5File file(FILE_NAME, H5F_ACC_TRUNC);

		// Continuous chunks dataset
		const H5std_string CONTINUOUS_DATASET_NAME("continuous_chunks");

		const int      NC = continuous_count;
		const int      NX = storage->chunk_size();
		const int      NY = storage->chunk_size();
		const int	   NZ = storage->chunk_size();

		const int      RANK = 4;
		
		hsize_t     dimsf[4];
		dimsf[0] = NC;
		dimsf[1] = NX;
		dimsf[2] = NY;
		dimsf[3] = NZ;

		H5::DataSpace dataspace(RANK, dimsf);
		H5::IntType datatype(H5::PredType::NATIVE_INT);
		datatype.setOrder(H5T_ORDER_LE);
		H5::DataSet dataset = file.createDataSet(CONTINUOUS_DATASET_NAME, datatype, dataspace);

		// Continuous dataset hyperslab preparation
		hsize_t     offset[4];
		offset[0] = -1;
		offset[1] = 0;
		offset[2] = 0;
		offset[3] = 0;

		hsize_t     slab_dimsf[4] = { 1, 1, 1, 1 };
		H5::DataSpace mspace2(RANK, slab_dimsf);



		//Planar chunks dataset
		typedef struct s2_t {
			int axis;
			hvl_t offsets;
		} s2_t;

		const H5std_string AXIS("axis");
		const H5std_string OFFSETS("offsets");

		H5::VarLenType varlen_type(&H5::PredType::NATIVE_INT);
		H5::CompType mtype2(sizeof(s2_t));
		mtype2.insertMember(AXIS, HOFFSET(s2_t, axis), H5::PredType::NATIVE_INT);
		mtype2.insertMember(OFFSETS, HOFFSET(s2_t, offsets), varlen_type);
		const H5std_string PLANAR_DATASET_NAME("planar_chunks");

		const int      PLANAR_RANK = 1;
		hsize_t     planar_dimsf[1];
		planar_dimsf[0] = planar_count;
		H5::DataSpace planar_dataspace(PLANAR_RANK, planar_dimsf);
		H5::DataSet planar_dataset = file.createDataSet(PLANAR_DATASET_NAME, mtype2, planar_dataspace);
		hsize_t     planar_offset[1];
		planar_offset[0] = -1;
		hsize_t     planar_dims[1] = { 1 };
		H5::DataSpace planar_space(PLANAR_RANK, planar_dims);



		// Regions dataset
		const H5std_string REGIONS_DATASET_NAME("regions");

		hsize_t     regions_dimsf[1];
		regions_dimsf[0] = continuous_count + constant_count + planar_count;

		const int      REGION_RANK = 1;

		H5::DataSpace regions_dataspace(REGION_RANK, regions_dimsf);
		H5::PredType regions_datatype(H5::PredType::STD_REF_DSETREG);
		regions_datatype.setOrder(H5T_ORDER_LE);
		H5::DataSet regions_dataset = file.createDataSet(REGIONS_DATASET_NAME, regions_datatype, regions_dataspace);

		//  Regions dataset hyperslab preparation
		hsize_t     region_offset[1];
		region_offset[0] = -1;
		hsize_t     region_slab_dimsf[1] = { 1 };
		H5::DataSpace mspace3(REGION_RANK, region_slab_dimsf);

		// Continuous chunks region hyperslab preparation
		hsize_t     chunk_offset[4];
		const int CHUNK_RANK = 4;
		chunk_offset[0] = -1;
		chunk_offset[1] = 0;
		chunk_offset[2] = 0;
		chunk_offset[3] = 0;
		hsize_t     chunk_dimsf[4] = { 1, NX, NY, NZ };
		H5::DataSpace chunk_space(CHUNK_RANK, chunk_dimsf);

		// Planar chunks region hyperslab preparation 
		hsize_t     planar_chunk_offset[1];
		const int PLANAR_CHUNK_RANK = 1;
		planar_chunk_offset[0] = 1;
		hsize_t     planar_chunk_dimsf[1] = { 1 };
		H5::DataSpace planar_chunk_space(PLANAR_CHUNK_RANK, planar_chunk_dimsf);


		BEGIN_LOOP(size_t(0), nchunks_x, 0U, nchunks_y, 0U, nchunks_z)
			auto c = storage->get_chunk(ijk);

			if (c == nullptr) {
				std::cout << "Null pointer" << std::endl;

			}

			else {
				if (c->is_explicit() || c->is_constant()) {
					if (c->is_constant()) {
					std::cout << "Constant chunk" << std::endl;

				}

				else {
					std::cout << "Continuous chunk" << std::endl;

					offset[0]++;

					continuous_voxel_storage<bit_t>* convox = (continuous_voxel_storage<bit_t>*)c;
					size_t i0, j0, k0, i1, j1, k1;
					i0 = 0;
					j0 = 0;
					k0 = 0;
					convox->extents().tie(i1, j1, k1);

					BEGIN_LOOP_ZERO_2(make_vec<size_t>(i1, j1, k1))
						offset[1] = ijk.get(0);
					offset[2] = ijk.get(1);
					offset[3] = ijk.get(2);
					dataspace.selectHyperslab(H5S_SELECT_SET, slab_dimsf, offset);

					std::vector<int> hslab = { convox->Get(ijk) };
					dataset.write(hslab.data(), H5::PredType::NATIVE_INT, mspace2, dataspace);
					END_LOOP;

					region_offset[0]++;
					regions_dataspace.selectHyperslab(H5S_SELECT_SET, region_slab_dimsf, region_offset);

					chunk_offset[0]++;

					dataspace.selectHyperslab(H5S_SELECT_SET, chunk_dimsf, chunk_offset);

					hobj_ref_t inter[1];
					file.reference(&inter[0], "/continuous_chunks", dataspace, H5R_DATASET_REGION);
					regions_dataset.write(inter, H5::PredType::STD_REF_DSETREG, mspace3, regions_dataspace);
				}
			}

				else {
					std::cout << "Planar chunk" << std::endl;
					planar_voxel_storage<bit_t>* planvox = (planar_voxel_storage<bit_t>*)c;

					auto axis = planvox->axis();
					auto off = planvox->offsets();

					std::vector<int> offsets(off.begin(), off.end());

					planar_offset[0]++;
				
					planar_dataspace.selectHyperslab(H5S_SELECT_SET, planar_dims, planar_offset);
					std::vector<s2_t> input_vec;
					hvl_t varlen_offsets;

			
					varlen_offsets.len = offsets.size();

					varlen_offsets.p = offsets.data();
					s2_t compound_data;
					compound_data.axis = axis;
					compound_data.offsets = varlen_offsets;
					input_vec.push_back(compound_data);
					planar_dataset.write(&compound_data, mtype2, planar_space, planar_dataspace);


					region_offset[0]++;
					regions_dataspace.selectHyperslab(H5S_SELECT_SET, region_slab_dimsf, region_offset);

					planar_chunk_offset[0]++;

					dataspace.selectHyperslab(H5S_SELECT_SET, planar_chunk_dimsf, planar_chunk_offset);

					hobj_ref_t inter[1];
					file.reference(&inter[0], "/planar_chunks", planar_dataspace, H5R_DATASET_REGION);
					regions_dataset.write(inter, H5::PredType::STD_REF_DSETREG, mspace3, regions_dataspace);

				}
			}
		END_LOOP
	}
};






#endif
