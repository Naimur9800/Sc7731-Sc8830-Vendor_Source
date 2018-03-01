#!/bin/bash

#��fileĩβ��һЩ�س���ɾ��
function clear_file()
{
	declare -i end
	declare -i n
	file=$1
	cd $2
	n=0
	end=0
	file_t="$file""_bak"
	file_n="$file""_bak_b"    #������Ļ���
	file_m="$file""_bak_bak"  #computer lines
	sed 's/.$//' $file >$file_m  #ɾ��ÿ�е����һ���ַ�,������"\",��������д���

	cat $file_m | while read  line
	do
		n=n+1
		#echo $line
		#echo ${#line}
		#echo "n="$n
		if [ ${#line} -gt 0 ];then
			end=n
			echo $end >$file_t
		fi
	done
	end=0
	if [[ -f $file_t ]];then
		while read line
		do
	    end=$line
		done < $file_t
		rm $file_t -rf
	fi
	if [[ -f $file_m ]];then
		rm $file_m -rf
	fi
	echo "end="$end
	head -$end $file >$file_n
	mv $file_n $file
}

function add_to_file()
{
	#echo "add_to_file"
	file=$1
	cd $2
	substring=$3
	replacement=$4
	substring_2=0
	var2=$5
	var1=$6
	is_substring_2=0  #is_substring_2�����Ƿ���Ҫ�滻�ڶ����ַ���
	if [ $# -ge 9 ];then
		#echo "$8 $9"
		substring_2=$8
		replacement_2=$9
		is_substring_2=1
	fi
	is_add_tab=0  #�Ƿ��tab�Ʊ��,0��ʾ����Ҫ,��ʾ�ӵ�is_add_tab�к�ʼtab�Ʊ��
	if [ $# -ge 7 ];then
		#echo $7
		is_add_tab=$7
	fi

	declare -i n
	declare -i begin
	declare -i end
	declare -i m
	declare -i k  #k���ڼ�������п�ʼ��ʼ����tab�Ʊ��
	k=0

	#if [ $# -lt 2 ]
	n=0
	begin=0
	file_t="$file""_bak"
	file_n="$file""_bak_b"    #������Ļ���
	is_one_line='skip_one_line'


	cat $file | while read  line
	do
		#echo $line
		n=n+1
		echo "$line" |grep -qi "$var2""\>" >$file_n  #�ַ��������ȫ���
		m=$?
		if [ $begin -gt 0 ];then
			echo "$line" | grep -qi "$var1" >$file_n
			if [ $? -eq 0 ];then
				end=$n
				#echo $begin >$file_t
				echo $end >$file_t
				echo "find"" end=""$end"
				echo $line
				break
			fi
		elif [ $m -eq 0 ];then
			begin=$n
			echo "find"" begin=""$begin"
			echo $line
			echo "$var1" | grep "$is_one_line" >$file_n
			if [ $? -eq 0 ];then
				echo "only add one line"
				line=${line//$substring/$replacement}
				substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring"`
				replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement"`
				line=${line//$substring_b/$replacement_b}
				if [ $is_substring_2 -gt 0 ];then
					echo "$line" | grep -i "$substring_2" >$file_n
					if [ $? -eq 0 ];then
						line=${line//$substring_2/$replacement_2}
						substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring_2"`
						replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement_2"`
						line=${line//$substring_b/$replacement_b}
					fi
				fi
				sed -e "$n""a\\$line"  $file >$file_t
				cp $file_t $file
				rm $file_t -rf
				break
			fi
		fi
		#echo $n
	done


	end=0
	if [[ -f $file_t ]];then
		while read line
		do
	    end=$line
		done < $file_t
		rm $file_t -rf
	fi

	if [[ -f $file_n ]];then
		rm $file_n -rf
	fi

	echo "find end line is ""$end"
	begin=0
	if [ $end -gt 0 ];then
		cat $file | while read  line
		do
			echo "$line" | grep -qi "$var2""\>" >$file_n
			m=$?
			if [ $begin -gt 0 ];then
				n=n+1
				echo "$line" | grep "$substring" >$file_n
				if [ $? -eq 0 ];then
					#echo have
					line=${line//$substring/$replacement}
					#echo $line
				else
					substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring"`
					replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement"`
					line=${line//$substring_b/$replacement_b}
					#echo $line
				fi
				if [ $is_substring_2 -gt 0 ];then
					echo "$line" | grep "$substring_2" >$file_n
					if [ $? -eq 0 ];then
						line=${line//$substring_2/$replacement_2}
						#echo $substring$replacement
						#echo "my log aa "$line
					else
						substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring_2"`
						replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement_2"`
						line=${line//$substring_b/$replacement_b}
						#echo "my log bb "$line
					fi
				fi
				k=k+1
				if [ $is_add_tab -gt 0 ];then
					if [ $k -ge $is_add_tab ];then
						line_s="$line_s""\n\t$line"
					else
						line_s="$line_s""\n$line"
					fi
				else
					line_s="$line_s""\n$line"
				fi
				echo "$line" |grep -qi "$var1" >$file_n
				if [ $? -eq 0 ];then
					echo "begin to write to file"
					#echo $end
					#echo $line_s
					sed -e "$end""a\\$line_s"  $file >$file_t
					cp $file_t $file
					rm $file_t -rf
					break
				fi
			elif [ $m -eq 0 ];then
				n=n+1
				begin=$n
				echo "$line" | grep "$substring" >$file_n
				if [ $? -eq 0 ];then
					line=${line//$substring/$replacement}
					#echo log a ....
					echo $substring$replacement
					#echo $line
				else
					substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring"`
					replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement"`
					line=${line//$substring_b/$replacement_b}
					#echo log b....
					#echo $line
				fi

				if [ $is_substring_2 -gt 0 ];then
					echo "$line" | grep "$substring_2" >$file_n
					if [ $? -eq 0 ];then
						line=${line//$substring_2/$replacement_2}
						#echo $substring$replacement
						#echo "my log a "$line
					else
						substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring_2"`
						replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement_2"`
						line=${line//$substring_b/$replacement_b}
						#echo "my log b "$line
					fi
				fi

				if [ $k -gt $is_add_tab ];then
					line_s="\n\t$line"
				else
					line_s="\n$line"
				fi
				k=k+1
			else
				n=n+1
			fi
		done
	fi

	if [[ -f $file_n ]];then
		rm $file_n -rf
	fi
}

#�ô˺���,�ؼ��ֲ����������һ����,����ɾ��һ���ַ�,�����ִ���
function add_to_file_e()
{
	file=$1
	cd $2
	substring=$3
	replacement=$4
	var2=$5
	var1=$6
	have_second_end_string=0
	only_add_one_line=0

	if [ $# -ge 7 ];then
		temp=$7
		if [ ${#temp} -gt 1 ];then
			#second_end_str���ڵ��б��������var1���ڵ���,
			echo "second_end_str="$7  #�еڶ������ַ���,��һ�����ַ����ĺ����ǵڶ������ַ���,���������ڵڶ������ַ�������,�����ĵڶ������ַ�������Ϊ׼
			second_end_string=$7
			have_second_end_string=1
		fi
	fi
	is_substring_2=0  #is_substring_2�����Ƿ���Ҫ�滻�ڶ����ַ���
	substring_2=0
	if [ $# -ge 9 ];then
		#�����ӵ����滻�ڶ����ַ���
		#echo "$8 $9"
		substring_2=$8
		replacement_2=$9
		is_substring_2=1
	fi

	declare -i n
	declare -i begin
	declare -i end
	declare -i m

	#if [ $# -lt 2 ]
	n=0
	begin=0
	file_t="$file""_bak"
	file_m="$file""_bak_bak"  #computer lines
	file_n="$file""_bak_b"    #������Ļ���
	file_s="$file""_bak_s"    #�ҵ��ڶ������ַ���
	file_o="$file""_bak_o"    #�Ƿ�ֻ����һ��
	is_one_line='skip_one_line'

	sed 's/.$//' $file >$file_m  #ɾ��ÿ�е����һ���ַ�,������"\",��������д���

	cat $file_m | while read line >$file_n
	do
		#echo $line
		n=n+1
		#echo $n
		echo "$line" |grep -qi "$var2""\>" >$file_n   #�ַ��������ȫ���
		m=$?
		if [ $begin -gt 0 ];then
			#echo "$line"
			#echo $var1
			echo "$line" |grep -qi "$var1" >$file_n
			if [ $? -eq 0 ];then
				end=$n
				#echo $begin >$file_t
				echo $end >$file_t   #end������do...doneѭ�����ֵ,����ѭ�����ڲ�������,�ʱ��浽һ����ʱ�ļ�
				echo "find"" end=""$end"
				if [ $have_second_end_string -eq 1 ];then
					have_second_end_string=2
					var1=$second_end_string
					echo have_second_end_string
					continue
				elif [ $have_second_end_string -eq 2 ];then
					echo "find second_end_string:"$var1" in $line"
					have_second_end_string=3  #continue,�ñ�־3
					echo $have_second_end_string >$file_s
					continue
				elif [ $have_second_end_string -eq 3 ];then
					echo "find second_end_string:"$var1" in $line"
					continue   #continue,���ҵ����һ��
				fi
				break
			elif [ $have_second_end_string -eq 2 ];then  #û�ҵ��ڶ������ַ���,�Ե�һ�����ַ���Ϊ׼
				echo "not find second_end_string:"$var1
				have_second_end_string=0
				break
			elif [ $have_second_end_string -eq 3 ];then  #�����ǵڶ������ַ���,break
				echo "find second_end_string:"$var1" and break"
				break
			fi
		elif [ $m -eq 0 ];then
			begin=$n
			echo "find"" begin=""$begin"
			echo "$var1" | grep "$is_one_line" >$file_n
			if [ $? -eq 0 ];then
				echo "xxx only add one line"
				end=$n
				echo $end >$file_t
				only_add_one_line=1
				echo $only_add_one_line >$file_o
				break
			fi
		fi
	done


	end=0
	if [[ -f $file_t ]];then
		while read line
		do
	    end=$line
		done < $file_t
		rm $file_t -rf
	fi
	if [[ -f $file_s ]];then
		while read line
		do
	    have_second_end_string=$line
		done < $file_s
		rm $file_s -rf
		if [ $have_second_end_string -eq 3 ];then
			var1=$second_end_string  #�Եڶ������ַ���Ϊ׼
		fi
	fi
	if [[ -f $file_o ]];then
		while read line
		do
	    only_add_one_line=$line
		done < $file_o
		rm $file_o -rf
	fi

	if [[ -f $file_m ]];then
		rm $file_m -rf
	fi

	if [[ -f $file_n ]];then
		rm $file_n -rf
	fi

	echo "find end line is ""$end"
	begin=0
	if [ $end -gt 0 ];then
		cat $file | while read line
		do
			echo "$line" | grep -qi "$var2""\>" >$file_n   #�ַ��������ȫ���
			m=$?
			if [ $begin -gt 0 ];then
				n=n+1
				echo "$line" | grep "$substring" >$file_n
				if [ $? -eq 0 ];then
					#echo have
					line=${line//$substring/$replacement}
				else
					substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring"`
					replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement"`
					line=${line//$substring_b/$replacement_b}
				fi
				line_s_b=$line_s #����һ��
				line_s="$line_s""\n\t$line"
				echo "$line" |grep -qi "$var1" >$file_n
				if [ $? -eq 0 ];then
					if [ $have_second_end_string -eq 3 ];then
					#����,���ҵ������ȵ�second_end_string
						have_second_end_string=4
						#echo $line
						continue
					elif [ $have_second_end_string -eq 4 ];then
						#echo $line
						continue
					fi
					echo "begin to write to file"
					#echo $end
					#echo $line_s
					sed -e "$end""a\\$line_s"  $file >$file_t
					cp $file_t $file
					rm $file_t -rf
					break
				elif [ $have_second_end_string -eq 4 ];then
					#echo $line
					echo "second,begin to write to file,end=$end"
					line_s=$line_s_b  #����һ��line_s
					#echo $line_s
					sed -e "$end""a\\$line_s"  $file >$file_t
					cp $file_t $file
					rm $file_t -rf
					break
				fi
			elif [ $m -eq 0 ];then
				n=n+1
				begin=$n
				echo "$line" | grep -i "$substring" >$file_n
				if [ $? -eq 0 ];then
					line=${line//$substring/$replacement}
					substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring"`
					replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement"`
					line=${line//$substring_b/$replacement_b}
				fi
				if [ $only_add_one_line = 1 ];then
					#only add one line
					echo xxzzz
					if [ $is_substring_2 -gt 0 ];then
						echo "$line" | grep -i "$substring_2" >$file_n
						if [ $? -eq 0 ];then
							line=${line//$substring_2/$replacement_2}
							substring_b=`tr '[a-z]' '[A-Z]' <<<"$substring_2"`
							replacement_b=`tr '[a-z]' '[A-Z]' <<<"$replacement_2"`
							line=${line//$substring_b/$replacement_b}
						fi
					fi
					sed -e "$end""a\\$line"  $file >$file_t
					cp $file_t $file
					rm $file_t -rf
					break
				fi
				line_s="\n$line"
			else
				n=n+1
			fi
		done
	fi

	if [[ -f $file_n ]];then
		rm $file_n -rf
	fi
}

function board_for_kernel()
{
	DEBUG_S=debug
	if [ $# -gt 0 ];then
		echo "$1" | grep "$DEBUG_S"
		if [ $? -eq 0 ];then
			PLATFORM=scx15
			BOARD_NAME_R=sp7715ea
			BOARD_NAME_N=sp7715eaopenphone
			workdir=$PWD
		fi
	fi
	USE_INPUT_BOARD_MACRO=0
	have_dt_board=0

	echo -e "\n\nconfigure kernel begin......"
	#---------����*native_defconfig�ļ� begin-------
	cd $workdir
	cd "kernel/arch/arm/configs"

	TEMP=$BOARD_NAME_R"-native_defconfig"
	echo $TEMP
	TEMP1=$BOARD_NAME_N"-native_defconfig"
	if [  -f $TEMP ];then
		cp $TEMP $TEMP1
	else
	  #��Щboard��defconfig�ļ�ȡ��ΪBOARD_NAME_N_defconfig,����:sp5735ea_defconfig
		TEMP=$BOARD_NAME_R"_defconfig"
		TEMP1=$BOARD_NAME_N"_defconfig"
		if [  -f $TEMP ];then
			cp $TEMP $TEMP1
		else
			echo -e $BOARD_NAME_R"***_defconfig not exist,configure kernel fail!!!!"
			exit 0
		fi
	fi

	file=$TEMP1

	TEMP1=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
	TEMP2=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_N"`
	if [ -z "`grep -Hrn --include="$TEMP" $TEMP1 ./`" ]
	then
		#��defconfig�ļ���δ�ҵ�����Ǹ��ַ���
		echo -e "CONFIG_MACH_""$TEMP1"" is not defined in ""$TEMP""!!!"
		echo -e "must input board macro that is defined in ""$TEMP"
		g_count=0

		BOARD_MACRO_IN_KERNLE=
		while true
		do
			input_board_macro_for_kernel
			g_count=$g_count+1
			#echo $BOARD_MACRO_IN_KERNLE
			#��ת����Сд
			BOARD_MACRO_IN_KERNLE=`tr '[A-Z]' '[a-z]' <<<"$BOARD_MACRO_IN_KERNLE"`
			TEMP1=`tr '[a-z]' '[A-Z]' <<<"$BOARD_MACRO_IN_KERNLE"`
			#echo $TEMP1
			if [ -z "`grep -Hrn --include="$TEMP" $TEMP1 ./`" ];then
				echo -e "do not find ""CONFIG_MACH_""$TEMP1"" in ""$TEMP"
				if [ $g_count -gt 3 ];then
					echo -e "input times have exceed 3,configure kernel fail and exit!!"
					exit 0
				fi
				echo -e "please input again"
			else
				TEMP1="CONFIG_MACH_""$TEMP1""=y"
				TEMP2="CONFIG_MACH_""$TEMP2""=y"
				sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`
				USE_INPUT_BOARD_MACRO=1
				echo -e "find ""CONFIG_MACH_""$TEMP1"" in ""$TEMP"
				echo -e "input ok,continue configure kernel"
				break
			fi
		done
	else
		#echo "find $TEMP1"
		TEMP1="CONFIG_MACH_""$TEMP1""=y"
		TEMP2="CONFIG_MACH_""$TEMP2""=y"
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`
	fi

	#clone dt_defconfig
	TEMP_=$BOARD_NAME_R"-dt_defconfig"
	#echo "xxx"$TEMP_
	TEMP_1=$BOARD_NAME_N"-dt_defconfig"
	if [  -f $TEMP_ ];then
		cp $TEMP_ $TEMP_1
		have_dt_board=1
		file=$TEMP_1
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`
	fi

	#clone dt_defconfig
	TEMP_=$BOARD_NAME_R"_dt_defconfig"
	#echo "xxx"$TEMP_
	TEMP_1=$BOARD_NAME_N"_dt_defconfig"
	if [  -f $TEMP_ ];then
		cp $TEMP_ $TEMP_1
		have_dt_board=1
		file=$TEMP_1
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`
	fi
	#---------����*native_defconfig�ļ� end-------
	
	#---------����*dts�ļ� begin-------
	cd $workdir
	cd "kernel/arch/arm/boot/dts"
	file="sprd-""$PLATFORM""_""$BOARD_NAME_R"".dts"
	#echo $file
	if [  -f $file ];then
		TEMP_1="sprd-""$PLATFORM""_""$BOARD_NAME_N"".dts"
		cp $file $TEMP_1
	fi
	#---------����*dts�ļ� end-------
	
	#---------�޸�kernel/arch/arm/boot/dts/Makefile begin-------
	#echo $have_dt_board
	if [ $have_dt_board = 1 ];then
		echo "modify kernel/arch/arm/boot/dts/Makefile"
		cd $workdir
		file=Makefile
		replacement=$BOARD_NAME_N
		path="kernel/arch/arm/boot/dts"
		end_string='skip_one_line'
		if [ $USE_INPUT_BOARD_MACRO -gt 0 ];then
			#kconfig�е�board����Ϊ���淶,ʹ����������Ĵ�
			#echo "kernel/arch/arm/boot/dts/Makefile do not modify ok,please modify it after"
			substring=$BOARD_MACRO_IN_KERNLE
			begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_MACRO_IN_KERNLE"`
			add_to_file_e  $file $path $substring $replacement $begin_string $end_string 0 $BOARD_NAME_R $replacement
		else
			substring=$BOARD_NAME_R
			begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
			add_to_file_e $file $path $substring $replacement $begin_string $end_string
		fi
	fi
	#---------�޸�kernel/arch/arm/boot/dts/Makefile end-------


	#---------�޸�kernel\arch\arm\mach-sc\Makefile�ļ� begin-------
	cd $workdir
	file=Makefile
	replacement=$BOARD_NAME_N
	path="kernel/arch/arm/mach-sc"
	end_string='skip_one_line'
	if [ $USE_INPUT_BOARD_MACRO -gt 0 ];then
	#kconfig�е�board����Ϊ���淶,ʹ����������Ĵ�
		substring=$BOARD_MACRO_IN_KERNLE
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_MACRO_IN_KERNLE"`
		add_to_file $file $path $substring $replacement $begin_string $end_string 0 $BOARD_NAME_R $replacement
	else
		substring=$BOARD_NAME_R
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		add_to_file $file $path $substring $replacement $begin_string $end_string
	fi
	#---------�޸�kernel\arch\arm\mach-sc\Makefile�ļ� end-------


	#---------����*board-BOARD_NAME.c�ļ� begin-------
	cd $workdir
	cd kernel/arch/arm/mach-sc
	TEMP1="board-"$BOARD_NAME_R".c"
	TEMP2="board-"$BOARD_NAME_N".c"
	cp $TEMP1 $TEMP2

	cd include/mach
	TEMP1="__board-"$BOARD_NAME_R".h"
	TEMP2="__board-"$BOARD_NAME_N".h"
	cp $TEMP1 $TEMP2
	file=$TEMP2
	echo $file

	TEMP='sp'
	TEMP1=${BOARD_NAME_R#*$TEMP}
	TEMP1=`tr '[a-z]' '[A-Z]' <<<"$TEMP1"`
	TEMP1=$TEMP1"_"
	echo $TEMP1
	TEMP2=${BOARD_NAME_N#*$TEMP}
	TEMP2=`tr '[a-z]' '[A-Z]' <<<"$TEMP2"`
	TEMP2=$TEMP2"_"
	echo $TEMP2
	sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`
	#---------����*board-BOARD_NAME.c�ļ�end-------

	#---------�޸�board.h�ļ� begin-------
	cd $workdir
	file=board.h
	replacement=$BOARD_NAME_N
	path="kernel/arch/arm/mach-sc/include/mach"
	end_string=endif
	if [ $USE_INPUT_BOARD_MACRO -gt 0 ];then
	#kconfig�е�board����Ϊ���淶,ʹ����������Ĵ�
		substring=$BOARD_MACRO_IN_KERNLE
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_MACRO_IN_KERNLE"`
		add_to_file $file $path $substring $replacement $begin_string $end_string 0 $BOARD_NAME_R $replacement
	else
		substring=$BOARD_NAME_R
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		add_to_file $file $path $substring $replacement $begin_string $end_string
	fi
	#---------�޸�board.h�ļ� end-------

	#---------�޸�Kconfig�ļ� begin-------
	cd $workdir
	file=Kconfig
	replacement=$BOARD_NAME_N
	path="kernel/arch/arm/mach-sc"
	end_string='serial'
	if [ $USE_INPUT_BOARD_MACRO -gt 0 ];then
	#kconfig�е�board����Ϊ���淶,ʹ����������Ĵ�
		substring=$BOARD_MACRO_IN_KERNLE
		begin_string=`tr '[a-z]' '[A-Z]' <<<"MACH_$BOARD_MACRO_IN_KERNLE"`
		add_to_file $file $path $substring $replacement $begin_string $end_string 2 $BOARD_NAME_R $replacement
	else
		substring=$BOARD_NAME_R
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		add_to_file $file $path $substring $replacement $begin_string $end_string 2
	fi
	#---------�޸�Kconfig�ļ� end-------


	#---------����'|| defined(CONFIG_MACH_BOARD_NAME_N)' begin-------
	cd $workdir
	cd kernel/
	if [ $USE_INPUT_BOARD_MACRO -gt 0 ];then
	#kconfig�е�board����Ϊ���淶,ʹ����������Ĵ�
		BOARD_NAME_R_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_MACRO_IN_KERNLE"`
	else
		BOARD_NAME_R_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
	fi
	BOARD_NAME_N_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_N"`
	substring="(CONFIG_MACH_$BOARD_NAME_R_b)"
	addstring=" || defined(CONFIG_MACH_$BOARD_NAME_N_b)"
	replacestring="$substring$addstring"
	echo -e $substring
	echo -e $addstring
	echo -e $replacestring
	#if [ `grep -Hrn  $substring ./` ];then
	#echo ok
	#fi
	if [ -z "`grep -Hrn --exclude-dir=".git" --include="*.c" --include="*.h" $substring ./`" ]
	then
		echo "NULL"
	else
		#sed -i "/$substring/s/$/$addstring/;" `grep $substring -rl --include="*.*" ./`
		#sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="*.*" ./`
		sed -i s/"$substring"/"$replacestring"/g `grep $substring -rl --include="*.*" ./`
		echo "===========================notice========================================"
		echo "===========================notice========================================"
		echo "maybe add \""$addstring"\" words!"
		echo "please check that if need!!!!!"
	fi

  #---------����'|| (defined CONFIG_MACH_BOARD_NAME_N)' begin-------
	substring="(defined CONFIG_MACH_$BOARD_NAME_R_b)"
	addstring=" || (defined CONFIG_MACH_$BOARD_NAME_N_b)"
	replacestring="$substring$addstring"
	echo -e $substring
	echo -e $addstring
	echo -e $replacestring
	#if [ `grep -Hrn  $substring ./` ];then
	#echo ok
	#fi
	if [ -z "`grep -Hrn --exclude-dir=".git" --include="*.c" --include="*.h" $substring ./`" ]
	then
		echo "NULL"
	else
		#sed -i "/$substring/s/$/$addstring/;" `grep $substring -rl --include="*.*" ./`
		#sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="*.*" ./`
		sed -i s/"$substring"/"$replacestring"/g `grep $substring -rl --include="*.*" ./`
		echo "===========================notice========================================"
		echo "===========================notice========================================"
		echo "maybe add \""$addstring"\" words!"
		echo "please check that if need!!!!!"
	fi
	#---------����'|| defined(CONFIG_MACH_BOARD_NAME_N)' end-------

	echo "configure kernel end!"
}

function board_for_uboot()
{
	DEBUG_S=debug
	if [ $# -gt 0 ];then
		echo "$1" | grep "$DEBUG_S"
		if [ $? -eq 0 ];then
			PLATFORM=scx15
			BOARD_NAME_R=sp7715ea
			BOARD_NAME_N=sp7715eaopenphone
			workdir=$PWD
		fi
	fi

	#echo $BOARD_NAME_N

	PATH_R="u-boot/board/spreadtrum/""$BOARD_NAME_R"
	PATH_N="u-boot/board/spreadtrum/""$BOARD_NAME_N"

	echo -e "\n\nconfigure u-boot begin......"

	while [ 1 ]
	do
		if [ ! -d $PATH_R ];then
			echo "$PATH_R not exist,configure u-boot fail!"
			break
		fi

		if [  -d $PATH_N ];then
			echo "$PATH_N exist,will rm it"
			rm $PATH_N -rf
		fi
		mkdir $PATH_N
		cp $PATH_R/* $PATH_N/ -rf

		PATH_M="u-boot/include/configs/"
		file_r="$PATH_M""$BOARD_NAME_R"".h"
		file_n="$PATH_M""$BOARD_NAME_N"".h"

		if [ ! -f $file_r ];then
			echo "$file_r not exist,configure u-boot fail!"
			break
		fi

		cp $file_r $file_n

		BOARD_NAME_R_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		#echo $BOARD_NAME_R_b
		BOARD_NAME_N_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_N"`
		#sed -i s/$BOARD_NAME_R_b/$BOARD_NAME_N_b/g `grep $BOARD_NAME_N_b $file_n`
		#echo $BOARD_NAME_N
		#echo $PATH_M
		sed -i s/$BOARD_NAME_R_b/$BOARD_NAME_N_b/g `grep $BOARD_NAME_R_b -rl --include="$BOARD_NAME_N"".h" $PATH_M`


	#---------�޸�Makefile�ļ� begin-------
		cd $workdir
		file=Makefile
		substring="$BOARD_NAME_R"
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		begin_string="$begin_string""_config"
		replacement="$BOARD_NAME_N"
		path="./u-boot"
		end_string='armv7'
		add_to_file_e $file $path $substring $replacement $begin_string $end_string
	#---------�޸��ļ� end-------


	#---------����'|| defined(CONFIG_BOARD_NAME_N)' begin-------
		cd $workdir
		cd u-boot/
		BOARD_NAME_R_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		BOARD_NAME_N_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_N"`
		substring="(CONFIG_$BOARD_NAME_R_b)"
		addstring=" || defined(CONFIG_$BOARD_NAME_N_b)"
		replacestring=$substring$addstring
		#echo -e $substring
		#echo -e $addstring
		if [ -z "`grep -Hrn --exclude-dir=".git" --include="*.c" --include="*.h" $substring ./`" ]
		then
			echo "NULL"
		else
			#sed -i "/$substring/s/$/$addstring/;" `grep $substring -rl --include="*.*" ./`
			sed -i s/"$substring"/"$replacestring"/g `grep $substring -rl --include="*.*" ./`
			echo "===========================notice========================================"
			echo "===========================notice========================================"
			echo "maybe add \"|| defined(CONFIG_$BOARD_NAME_N_b)\" words!"
			echo "please check that if need!!!!!"
		fi
	#---------����'|| defined(CONFIG_BOARD_NAME_N)' end-------

	break
	done

	echo "configure u-boot OK!"
}

function board_for_chipram()
{
	DEBUG_S=debug
	if [ $# -gt 0 ];then
		echo "$1" | grep "$DEBUG_S"
		if [ $? -eq 0 ];then
			PLATFORM=scx15
			BOARD_NAME_R=sp7715ea
			BOARD_NAME_N=sp7715eaopenphone
			workdir=$PWD
		fi
	fi

	#echo $BOARD_NAME_N

	echo -e "\n\nconfigure chipram begin......"

	while [ 1 ]
	do
		PATH_ROOT="chipram"
		if [ ! -d $PATH_ROOT ];then
			echo "$PATH_ROOT not exist,configure chipram fail!"
			break
		fi

		PATH_M="chipram/include/configs/"
		file_r="$PATH_M""$BOARD_NAME_R"".h"
		file_n="$PATH_M""$BOARD_NAME_N"".h"

		if [ ! -f $file_r ];then
			echo "$file_r not exist,configure chipram fail!"
			break
		fi

		cp $file_r $file_n

		BOARD_NAME_R_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		#echo $BOARD_NAME_R_b
		BOARD_NAME_N_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_N"`
		#sed -i s/$BOARD_NAME_R_b/$BOARD_NAME_N_b/g `grep $BOARD_NAME_N_b $file_n`
		#echo $BOARD_NAME_N
		#echo $PATH_M
		sed -i s/$BOARD_NAME_R_b/$BOARD_NAME_N_b/g `grep $BOARD_NAME_R_b -rl --include="$BOARD_NAME_N"".h" $PATH_M`


	#---------�޸�Makefile�ļ� begin-------
		cd $workdir
		file=Makefile
		substring="$BOARD_NAME_R"
		begin_string=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		begin_string="$begin_string""_config"
		replacement="$BOARD_NAME_N"
		path="./chipram"
		end_string='armv7'
		s_e_end_string='(obj)include'
		add_to_file_e $file $path $substring $replacement $begin_string $end_string $s_e_end_string
	#---------�޸��ļ� end-------


	#---------����'|| defined(CONFIG_BOARD_NAME_N)' begin-------
		cd $workdir
		cd chipram/
		BOARD_NAME_R_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_R"`
		BOARD_NAME_N_b=`tr '[a-z]' '[A-Z]' <<<"$BOARD_NAME_N"`
		substring="(CONFIG_$BOARD_NAME_R_b)"
		addstring=" || defined(CONFIG_$BOARD_NAME_N_b)"
		replacestring=$substring$addstring
		#echo -e $substring
		#echo -e $addstring
		if [ -z "`grep -Hrn --exclude-dir=".git" --include="*.c" --include="*.h" $substring ./`" ]
		then
			echo "NULL"
		else
			#sed -i "/$substring/s/$/$addstring/;" `grep $substring -rl --include="*.*" ./`
			sed -i s/"$substring"/"$replacestring"/g `grep $substring -rl --include="*.*" ./`
			echo "===========================notice========================================"
			echo "===========================notice========================================"
			echo "maybe add \"|| defined(CONFIG_$BOARD_NAME_N_b)\" words!"
			echo "please check that if need!!!!!"
		fi
	#---------����'|| defined(CONFIG_BOARD_NAME_N)' end-------

	break
	done

	echo "configure chipram OK!"
}


function board_for_device()
{
	DEBUG_S=debug
	if [ $# -gt 0 ];then
		echo "$1" | grep "$DEBUG_S"
		if [ $? -eq 0 ];then
			PLATFORM=scx15
			BOARD_NAME_R=sp7715ea
			BOARD_NAME_N=sp7715eaopenphone
			workdir=$PWD
		fi
	fi

	export PATH_R="device/sprd/$PLATFORM""_""$BOARD_NAME_R"

	#echo $PATH_R

	if [ ! -d $PATH_R ];then
		echo "reference board not exist,please check againxxx!"
		exit 0
	fi

	echo "configure device begin......"

	PATH_N="device/sprd/$PLATFORM""_""$BOARD_NAME_N"
	if  [ -d $PATH_N ];then
		rm $PATH_N -rf
	fi

	mkdir $PATH_N

	cp $PATH_R/* $PATH_N/ -rf

	cd $PATH_N

	if  [ $BOARD_TYPE = 1 ];then
		for filename in *$BOARD_NAME_R*
		do
		#echo $filename
		newname=`echo $filename | sed -n "s/$BOARD_NAME_R/$BOARD_NAME_N/p"`
		#echo "n="$newname
		mv $filename $newname
		done
	elif [ $BOARD_TYPE = 2 ];then
		TEMP=$PLATFORM"_""$BOARD_NAME_R""plus.mk"
		if [ ! -f $TEMP ];then
			echo "$TEMP"" not exist,maybe something error!\nplease check!"
			exit 0
		fi
		TEMP1="xxx122222"
		mkdir $TEMP1
		TEMP2=$PLATFORM"_""$BOARD_NAME_N"".mk"
		cp $TEMP $TEMP1/$TEMP2
		find -maxdepth 1 -name "*$BOARD_NAME_R*" | xargs rm -rf
		cp $TEMP1/* ./
		rm $TEMP1 -rf
	else
		echo "maybe choice error before"
		exit 0
	fi

	#---------�滻board���ؼ���begin----------------
	sed -i s/$BOARD_NAME_R/$BOARD_NAME_N/g `grep $BOARD_NAME_R -rl --include="*.*" ./`

	TEMP=$BOARD_NAME_R
	TEMP1=`tr '[a-z]' '[A-Z]' <<<"$TEMP"`

	TEMP=$BOARD_NAME_N
	TEMP2=`tr '[a-z]' '[A-Z]' <<<"$TEMP"`

	sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="*.*" ./`
	#---------�滻board���ؼ���end--------------------


	#---------ɾ��vendorsetup.sh AndroidProducts������������board begin-------
	if [ $BOARD_TYPE = 2 ];then  #clone trisim��board����Ҫɾ,normal board��UUI�ȹ��̶��ᱻclone
		file=vendorsetup.sh
		if [ $BOARD_TYPE = 1 ];then
			TEMP=asgii77jusua778
			TEMP1=$PLATFORM"_""$BOARD_NAME_N""base-"
			TEMP_1=$PLATFORM"_""$BOARD_NAME_N""base_dt-"
			#�������ⲻ�ظ����ַ���
			TEMP_=ad899mmkkk
		fi
		TEMP2=$PLATFORM"_""$BOARD_NAME_N""plus-"
		TEMP3=kjjs8j8jjshh7
	
		TEMP_2=$PLATFORM"_""$BOARD_NAME_N""plus_dt-"
		TEMP_3=iujk9990iuk
	
		if [ $BOARD_TYPE = 1 ];then
			sed -i s/$TEMP1/$TEMP/g `grep $TEMP1 -rl --include="$file" ./`
			sed -i s/$TEMP_1/$TEMP_/g `grep $TEMP_1 -rl --include="$file" ./`
		fi
		sed -i s/$TEMP2/$TEMP3/g `grep $TEMP2 -rl --include="$file" ./`
		sed -i s/$TEMP_2/$TEMP_3/g `grep $TEMP_2 -rl --include="$file" ./`
	
		sed -i -e /$BOARD_NAME_N/d $file
	
		if [ $BOARD_TYPE = 1 ];then
			sed -i s/$TEMP/$TEMP1/g `grep $TEMP -rl --include="$file" ./`
			sed -i s/$TEMP_/$TEMP_1/g `grep $TEMP_ -rl --include="$file" ./`
		fi
		sed -i s/$TEMP3/$TEMP2/g `grep $TEMP3 -rl --include="$file" ./`
		sed -i s/$TEMP_3/$TEMP_2/g `grep $TEMP_3 -rl --include="$file" ./`
	
		file=AndroidProducts.mk
		if [ $BOARD_TYPE = 1 ];then
			#TEMP=asgii77jusua778
			TEMP1=$PLATFORM"_""$BOARD_NAME_N""base.mk"
			TEMP_1=$PLATFORM"_""$BOARD_NAME_N""base_dt.mk"
		fi
		TEMP2=$PLATFORM"_""$BOARD_NAME_N""plus.mk"
		TEMP_2=$PLATFORM"_""$BOARD_NAME_N""plus_dt.mk"
		#TEMP3=kjjs8j8jjshh7
	
		if [ $BOARD_TYPE = 1 ];then
			sed -i s/$TEMP1/$TEMP/g `grep $TEMP1 -rl --include="$file" ./`
			sed -i s/$TEMP_1/$TEMP_/g `grep $TEMP_1 -rl --include="$file" ./`
		fi
		sed -i s/$TEMP2/$TEMP3/g `grep $TEMP2 -rl --include="$file" ./`
		sed -i s/$TEMP_2/$TEMP_3/g `grep $TEMP_2 -rl --include="$file" ./`
	
		#ɾ������BOARD_NAME_N*.mk����,��Ҫ���µ�board�����滻�ַ���
		sed -i -e /"$BOARD_NAME_N".*".mk"\/d $file
	
		#�����µ�board�Ļ�ȥ,��ԭ
		if [ $BOARD_TYPE = 1 ];then
			sed -i s/$TEMP/$TEMP1/g `grep $TEMP -rl --include="$file" ./`
			sed -i s/$TEMP_/$TEMP_1/g `grep $TEMP_ -rl --include="$file" ./`
		fi
		sed -i s/$TEMP3/$TEMP2/g `grep $TEMP3 -rl --include="$file" ./`
		sed -i s/$TEMP_3/$TEMP_2/g `grep $TEMP_3 -rl --include="$file" ./`
	fi
	#---------ɾ��vendorsetup.sh AndroidProducts������������board end-------

	#---------ɾ��AndroidProducts����\�ַ�begin-------
	clear_file vendorsetup.sh .
	TEMP=`tail -1 $file`
	TEMP1='\'
	TEMP2=${#TEMP}-1
	TEMP3=${TEMP:$TEMP2:1}

	if [[ $TEMP3 == '\' ]];then
		#echo ''\' is del'
		sed -i '$s/.$//' $file
		TEMP2=${#TEMP}-2
		TEMP3=${TEMP:$TEMP2:1}
		if [[ $TEMP3 == ' ' ]];then
		sed -i '$s/.$//' $file
		fi
	fi
	#---------ɾ��AndroidProducts����\�ַ�end-------

	if [ $BOARD_TYPE = 2 ];then
		TEMP1="$BOARD_NAME_N""plus"
		sed -i s/$TEMP1/$BOARD_NAME_N/g `grep $TEMP1 -rl --include="*.mk" ./`
		sed -i s/$TEMP1/$BOARD_NAME_N/g `grep $TEMP1 -rl --include="*.sh" ./`

		file=$PLATFORM"_""$BOARD_NAME_N"".mk"
		TEMP1="ro.msms.phone_count=2"
		TEMP2="ro.msms.phone_count=3"
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`

		TEMP1="persist.msms.phone_count=2"
		TEMP2="persist.msms.phone_count=3"
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`

		TEMP1="ro.modem.w.count=2"
		TEMP2="ro.modem.w.count=3"
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`

		TEMP1="ro.modem.t.count=2"
		TEMP2="ro.modem.t.count=3"
		sed -i s/$TEMP1/$TEMP2/g `grep $TEMP1 -rl --include="$file" ./`
	fi

	#---------����vendor/sprd/open-source/res/productinfoĿ¼�µ�ini�ļ�begin-------
	cd $workdir
	cd "vendor/sprd/open-source/res/productinfo"
	if [ $BOARD_TYPE = 1 ];then
		TEMP1=$PLATFORM"_""$BOARD_NAME_R""plus""_connectivity_configure.ini"
		TEMP2=$PLATFORM"_""$BOARD_NAME_N""plus""_connectivity_configure.ini"
		if [ -f $TEMP1 ];then
			cp $TEMP1 $TEMP2
		fi
		TEMP1=$PLATFORM"_""$BOARD_NAME_R""base""_connectivity_configure.ini"
		TEMP2=$PLATFORM"_""$BOARD_NAME_N""base""_connectivity_configure.ini"
		if [ -f $TEMP1 ];then
			cp $TEMP1 $TEMP2
		fi
	elif [ $BOARD_TYPE = 2 ];then
		TEMP1=$PLATFORM"_""$BOARD_NAME_R""plus""_connectivity_configure.ini"
		TEMP2=$PLATFORM"_""$BOARD_NAME_N""_connectivity_configure.ini"
		if [ -f $TEMP1 ];then
			cp $TEMP1 $TEMP2
		fi
	fi
	#---------����vendor/sprd/open-source/res/productinfoĿ¼�µ�ini�ļ�end-------
	echo "configure device end!"
}

function select_Platform_and_BoardType()
{
	echo "==================================================================="
	echo "please select:"
	echo "1. scx15 for dolphin and normal(for example:base or plus)"
	echo "2. scx15 for dolphin and trisim(three sim cards)"
	echo "3. scx35 for shark(tshark or 9620) and normal(for example:base or plus)"
	echo "4. scx35 for shark(tshark or 9620) and trisim(three sim cards)"
	echo "5. scx35l for sharkl(9630)"
	echo "==================================================================="
	read BOARD_PLATFORM_TYPE
}

function input_board_macro_for_kernel()
{
	echo "please input board macro in the file of defconfig"
	echo "for example:"
	echo "if CONFIG_MACH_SPX35EC,please input SPX35EC or spx35ec"
	echo "if CONFIG_MACH_SP7730GA,please input SP7730GA or sp7730ga"
	read BOARD_MACRO_IN_KERNLE
}

#�������� is here
workdir=$PWD
declare -i g_count

if [ ! -d "./kernel" ];then
	echo "board_clone.sh maybe is not in correct dir,please put it to android top dir!!"
	exit 0
fi

if [ ! -d "./u-boot" ];then
	echo "board_clone.sh maybe is not in correct dir,please put it to android top dir!!"
	exit 0
fi

if [ ! -d "./device/sprd" ];then
	echo "board_clone.sh maybe is not in correct dir,please put it to android top dir!!"
	exit 0
fi

g_count=0
BOARD_PLATFORM_TYPE=
while true
do
	select_Platform_and_BoardType
	if (echo -n $BOARD_PLATFORM_TYPE | grep -q -e "^[1-9][1-9]*$")
	then
		if [ "$BOARD_PLATFORM_TYPE" -gt "0" ]
		then
			case $BOARD_PLATFORM_TYPE in
				1)PLATFORM="scx15"
				BOARD_TYPE=1
				echo "you have choose scx15 and normal board!!"
				break
				;;
				2)PLATFORM="scx15"
				BOARD_TYPE=2
				echo "you have choose scx15 and trisim board!!"
				break
				;;
				3)PLATFORM="scx35"
				BOARD_TYPE=1
				echo "you have choose scx35 and normal board!!"
				break
				;;
				4)PLATFORM="scx35"
				BOARD_TYPE=2
				echo "you have choose scx35 and trisim board!!"
				break
				;;
				5)PLATFORM="scx35l"
				BOARD_TYPE=1
				echo "you have choose scx35l(9630) board!!"
				break
				;;
				*)echo "Invalid choice !"
			esac
		fi
	else
		echo  "you don't hava choose platform!"
	fi
	g_count=$g_count+1
	if [ $g_count -ge 3 ];then
		echo -e "select_Platform_and_BoardType fail,and exit!!!"
		exit 0
	else
		echo -e "please select again"
	fi
done

echo $PLATFORM
echo $"board type is ""$BOARD_TYPE"

if [ $# -lt 2 ]
then
	echo "$PLATFORM" | grep "scx15"
	if [ $? -eq 0 ];then
		echo "please input reference board name,for example:sp7715ea or sp7715ga or sp8815ga or 6815ea"
		TEMP="please input new board name,for example:sp7715ed or sp7715gc or sp8815gd or sp6815ef"
	else
		echo "please input reference board name,for example:sp7730ec or sp8830ec or sp7730gea or sp7730gga or sc9620openphone or sp7731gea or sp9630ea"
		TEMP="please input new board name,for example:sp7730ed or sp8830ef or sp7730gee or sp7730ggb or sc9620openphone_zt or sp7731gec or sp9630eb"
	fi
	read BOARD_NAME_R
	if [ $BOARD_TYPE = 2 ];then
		echo "please input new board name,for example:"$BOARD_NAME_R"trisim"
	else
		echo $TEMP
	fi
	read BOARD_NAME_N
else
	BOARD_NAME_R=$1
	BOARD_NAME_N=$2
fi

#��ת����Сд,��������Сд����
BOARD_NAME_R=`tr '[A-Z]' '[a-z]' <<<"$BOARD_NAME_R"`
BOARD_NAME_N=`tr '[A-Z]' '[a-z]' <<<"$BOARD_NAME_N"`

export PATH_R="device/sprd/$PLATFORM""_""$BOARD_NAME_R"

echo $PATH_R

if [ ! -d $PATH_R ];then
	echo "$PATH_R not exist!"
	echo "reference board not exist,please check again!"
	exit 0
fi

cd $workdir
board_for_device

cd $workdir
board_for_kernel

cd $workdir
board_for_uboot

cd $workdir
board_for_chipram